#include <iostream>
#include <stdbool.h>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <tuple>

#include <boost/program_options.hpp>
//#define CUBLAS  // При компиляции через `-DCUBLAS`
#ifdef CUBLAS
    #include "cublas_v2.h"  // cuBLAS API
#endif


// --------------------------------------------------------------------------
// Класс для отображения матрицы в сжатом виде (свёртка по блокам)
class MatrixPrinter {
public:
    // Конструктор: matrix – одномерный массив размера N*N (row-major),
    // display_size – желаемый размер сжатой матрицы (например, 10).
    MatrixPrinter(const double* matrix, int N, int display_size)
        : display_size_(display_size)
    {
        int block_size = N / display_size;  // целочисленное деление, остаток отбрасывается
        compressed_.resize(display_size, std::vector<double>(display_size, 0.0));

        for (int i = 0; i < display_size; ++i) {
            int i_start = i * block_size;
            for (int j = 0; j < display_size; ++j) {
                int j_start = j * block_size;
                double sum = 0.0;
                int count = 0;
                // обходим все пиксели блока block_size x block_size
                for (int bi = 0; bi < block_size; ++bi) {
                    for (int bj = 0; bj < block_size; ++bj) {
                        sum += matrix[(i_start + bi) * N + (j_start + bj)];
                        ++count;
                    }
                }
                compressed_[i][j] = sum / count;
            }
        }
    }

    // Вывод сжатой матрицы в виде целых чисел (округлённых до ближайшего целого).
    // Каждое число занимает минимум 2 позиции (с ведущим пробелом при <10).
    void print_numeric() const {
        for (int i = display_size_ - 1; i >= 0; --i) {
            for (int j = 0; j < display_size_; ++j) {
                int val = static_cast<int>(std::round(compressed_[i][j]));
                printf("%2d ", val);
            }
            printf("\n");
        }
    }

    // Цветной вывод: каждый элемент отображается символом █,
    // цвет меняется от синего (значение 10) до жёлтого (30).
    void print_color() const {
        int max_dif = highest_value_ - lowest_value_;
        for (int i = display_size_ - 1; i >= 0; --i) {
            for (int j = 0; j < display_size_; ++j) {
                double val = compressed_[i][j];
                // ограничиваем диапазон [10, 30]
                if (val < lowest_value_)  val = lowest_value_;
                if (val > highest_value_) val = highest_value_;
                double t = (val - lowest_value_) / max_dif;          // 0..1
                int r = static_cast<int>(255.0 * t);
                int g = static_cast<int>(255.0 * t);
                int b = static_cast<int>(255.0 * (1.0 - t));
                // ANSI True Color для символа █
                printf("\033[38;2;%d;%d;%dm██\033[0m", r, g, b);
            }
            printf("\n");
        }
    }

private:
    int display_size_;
    int lowest_value_  = 10;
    int highest_value_ = 30;
    std::vector<std::vector<double>> compressed_;  // сжатая матрица [display_size_][display_size_]
};



class HeatEquationSolver {
public:
    // Конструктор задаёт параметры задачи, но не запускает решение.
    HeatEquationSolver(int N, double epsilon, int max_iters)
        : N_(N), epsilon_(epsilon), max_iters_(max_iters),           // База
        inner_size_(N - 2), inner_total_(inner_size_ * inner_size_), // Константы внутренних точек
        inner_(inner_total_, 0.0), inner_new_(inner_total_, 0.0),    // Инициализация векторов внутренних точек
        bottom_(inner_size_), top_(inner_size_), left_(inner_size_), right_(inner_size_),  // Границы
        U_vec_(N * N, 0.0)                                           // Итоговая сетка
    {
        #ifdef CUBLAS
            cublasStatus_t stat = cublasCreate(&cublas_handle_);
            if (stat != CUBLAS_STATUS_SUCCESS) {
                std::cerr << "CUBLAS initialization failed\n";
            }
        #endif
    }

    ~HeatEquationSolver() {
        #ifdef CUBLAS
            if (cublas_handle_) cublasDestroy(cublas_handle_);
        #endif
    }

    // Основной метод – выполняет инициализацию и запускает метод Якоби.
    // Возвращает {число итераций, достигнутая ошибка, время счёта}.
    // После вызова solve() данные готовы на хосте, и их можно получить через get_solution().
    std::tuple<int, double, double> solve() {
        initialize_grid_();
        auto [iters, error, time] = solve_jacobi_();
        rebuild_full_grid();
        return {iters, error, time};
    }

    // Возвращает константный указатель на финальное поле (host, row-major).
    const double* get_solution() const {
        return U_vec_.data();
    }

private:
    int N_;
    double epsilon_;
    int max_iters_;
    int inner_size_;    // = N-2
    int inner_total_;   // = (N-2)^2
    const int check_interval_ = 50;

    // Данные, используемые в итерациях
    std::vector<double> inner_;       // текущее приближение внутренних точек
    std::vector<double> inner_new_;   // новое приближение внутренних точек
    std::vector<double> bottom_, top_, left_, right_; // границы (по N-2 элемента)

    // Полная сетка для вывода
    std::vector<double> U_vec_;

    #ifdef CUBLAS
    cublasHandle_t cublas_handle_ = nullptr;
    #endif

    // Угловые значения
    double lower_left_value = 10.0;
    double lower_right_value  = 20.0;
    double upper_left_value = 20.0;
    double upper_right_value = 35.0;


    // Инициализация сетки: углы + линейная интерполяция по границам.
    void initialize_grid_() {

        // Нижняя граница (i = 0, j = 1..N-2) 10 -> 20
        for (int j = 0; j < inner_size_; ++j) {
            bottom_[j] = lower_left_value + (lower_right_value - lower_left_value) * (j + 1) / (N_ - 1);
        }

        // Верхняя граница (i = N-1, j = 1..N-2) 20 -> 30
        for (int j = 0; j < inner_size_; ++j) {
            top_[j] = upper_left_value + (upper_right_value - upper_left_value) * (j + 1) / (N_ - 1);
        }

        // Левая граница (j = 0, i = 1..N-2) 10 -> 20
        for (int i = 0; i < inner_size_; ++i) {
            left_[i] = lower_left_value + (upper_left_value - lower_left_value) * (i + 1) / (N_ - 1);
        }

        // Правая граница (j = N-1, i = 1..N-2) 20 -> 30
        for (int i = 0; i < inner_size_; ++i) {
            right_[i] = lower_right_value + (upper_right_value - lower_right_value) * (i + 1) / (N_ - 1);
        }

        // Внутренние точки – нули
        std::fill(inner_.begin(), inner_.end(), 0.0);
        std::copy(inner_.begin(), inner_.end(), inner_new_.begin());
    }

    // Восстанавливает полную сетку U_vec_ из внутренних точек и границ
    void rebuild_full_grid() {

        // Углы
        U_vec_[0] = lower_left_value;                             // (0,0) – нижний левый
        U_vec_[N_ - 1] = lower_right_value;                       // (0, N-1) – нижний правый
        U_vec_[(N_ - 1) * N_ + (N_ - 1)] = upper_right_value;     // (N-1, N-1) – верхний правый
        U_vec_[(N_ - 1) * N_] = upper_left_value;                 // (N-1, 0) – верхний левый

        // Границы
        for (int j = 0; j < inner_size_; ++j) {
            U_vec_[1 + j] = bottom_[j];                      // нижняя строка
            U_vec_[(N_ - 1) * N_ + 1 + j] = top_[j];         // верхняя строка
        }
        for (int i = 0; i < inner_size_; ++i) {
            U_vec_[(i + 1) * N_] = left_[i];                 // левый столбец
            U_vec_[(i + 1) * N_ + (N_ - 1)] = right_[i];     // правый столбец
        }

        // Внутренние точки
        for (int i = 0; i < inner_size_; ++i) {
            for (int j = 0; j < inner_size_; ++j) {
                U_vec_[(i + 1) * N_ + (j + 1)] = inner_[i * inner_size_ + j];
            }
        }
    }

    // Основной итерационный метод
    std::tuple<int, double, double> solve_jacobi_() {
        // Указатели на данные для OpenACC
        double* inner = inner_.data();
        double* inner_new = inner_new_.data();
        double* bottom = bottom_.data();
        double* top = top_.data();
        double* left = left_.data();
        double* right = right_.data();

        // Временный массив для разности (только если CUBLAS)
        #ifdef CUBLAS
            std::vector<double> diff_vec(inner_total_);
            double* diff_arr = diff_vec.data();
        #endif



        int iter = 0;
        double error = 0.0;
        double elapsed_time = 0.0;
        auto start = std::chrono::high_resolution_clock::now();

        // OpenACC data region
        #pragma acc data copy(inner[0:inner_total_], inner_new[0:inner_total_]) \
                        copyin(bottom[0:inner_size_], top[0:inner_size_],       \
                               left[0:inner_size_], right[0:inner_size_])       \
                        create(diff_arr[0:inner_total_])
        {
            double max_error = 0.0;

            for (iter = 0; iter < max_iters_; ++iter) {
                // 1. Расчёт нового приближения (только внутренние точки)
                //#pragma acc parallel loop collapse(2) present(inner, inner_new, bottom, top, left, right)
                #pragma acc parallel loop gang  vector_length(128) present(inner, inner_new, bottom, top, left, right)
                for (int i = 0; i < inner_size_; ++i) {     // строка
                    #pragma acc loop vector
                    for (int j = 0; j < inner_size_; ++j) { // столбец
                    // table:    _value    = (condition)            ? side[idx] : inner_point[idx]
                        double down_val    = (i == 0)               ? bottom[j] : inner[(i - 1) * inner_size_ + j];
                        double up_val      = (i == inner_size_ - 1) ? top[j]    : inner[(i + 1) * inner_size_ + j];
                        double left_val    = (j == 0)               ? left[i]   : inner[i * inner_size_ + (j - 1)];
                        double right_val   = (j == inner_size_ - 1) ? right[i]  : inner[i * inner_size_ + (j + 1)];
                        inner_new[i * inner_size_ + j] = 0.25 * (up_val + down_val + left_val + right_val);
                    }
                }

                // 2. Вычисление максимальной ошибки (старое - новое приближение) &&
                // 3. Копирование inner_new -> inner
                if (iter % check_interval_ == 0) {
                    #ifdef CUBLAS
                    #pragma acc host_data use_device(inner, inner_new, diff_arr)
                    {
                        // diff = inner_new
                        cublasDcopy(cublas_handle_, inner_total_, inner_new, 1, diff_arr, 1);
                        // diff = diff - inner
                        double alpha = -1.0;
                        cublasDaxpy(cublas_handle_, inner_total_, &alpha, inner, 1, diff_arr, 1);
                        int max_idx;
                        cublasIdamax(cublas_handle_, inner_total_, diff_arr, 1, &max_idx);
                        cudaMemcpy(&max_error, diff_arr + (max_idx - 1), sizeof(double), cudaMemcpyDeviceToHost);
                        //  // max_error = diff_arr[max_idx - 1];
                        max_error = fabs(max_error);

                        // Update  inner = inner_new
                        cublasDcopy(cublas_handle_, inner_total_, inner_new, 1, inner, 1);
                    }
                    #else
                        max_error = 0.0;
                        // reduction вместо copy(max_error)
                        #pragma acc parallel loop  present(inner, inner_new) reduction(max:max_error)
                        for (int k = 0; k < inner_total_; ++k) {
                            double diff = fabs(inner_new[k] - inner[k]);
                            // if (diff > max_error) max_error = diff;
                            max_error = fmax(max_error, diff);
                            inner[k] = inner_new[k];  // Update
                        }
                    #endif

                    // 4. Проверка сходимости
                    if (max_error < epsilon_) break;
                }
            }
            error = max_error;
        } // end #pragma acc data


        auto end = std::chrono::high_resolution_clock::now();
        elapsed_time = std::chrono::duration<double>(end - start).count();

        return {iter, error, elapsed_time};
    }
};



int main(int argc, char* argv[]) {
    namespace po = boost::program_options;

    // Объявление параметров командной строки
    std::vector<int> grid_sizes;
    double epsilon = 1e-6;
    int max_iterations = 1000000;
    int display_size = 15;
    bool numeric_print = false;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Produce help message")
        ("grid-sizes,n", po::value<std::vector<int>>(&grid_sizes)->multitoken()->required(),
         "Grid sizes (one or more, e.g., 128 256 512 1024)")
        ("epsilon,e", po::value<double>(&epsilon)->default_value(1e-6, "1e-6"),
         "Convergence threshold")
        ("max-iter,m", po::value<int>(&max_iterations)->default_value(1000000),
         "Maximum number of iterations")
        ("display-size,g", po::value<int>(&display_size)->default_value(13),
         "Length of the compressed matrix displayed")
        ("numeric,n", po::value<bool>(&numeric_print)->default_value(false),
         "Print of numeric matrix");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }
        po::notify(vm);
    } catch (const po::error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    std::vector<std::tuple<int, int, double, double>> results;

    for (int N : grid_sizes) {
        HeatEquationSolver solver(N, epsilon, max_iterations);
        auto [iters, error, time] = solver.solve();
        std::cout << "Grid " << N << "x" << N << ": "
                    << iters << " iterations, error = "
                    << std::scientific << error
                    << ", time = " << std::fixed << time << " sec\n";

        MatrixPrinter printer(solver.get_solution(), N, display_size);
        if (numeric_print)  printer.print_numeric();
        printer.print_color();
    }

    return 0;
}
