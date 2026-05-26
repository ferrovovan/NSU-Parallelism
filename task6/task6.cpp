#include <iostream>
#include <stdbool.h>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <tuple>

#include <boost/program_options.hpp>


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
        for (int i = display_size_ - 1; i >= 0; --i) {
            for (int j = 0; j < display_size_; ++j) {
                double val = compressed_[i][j];
                // ограничиваем диапазон [10, 30]
                if (val < 10.0) val = 10.0;
                if (val > 30.0) val = 30.0;
                double t = (val - 10.0) / 20.0;          // 0..1
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
    std::vector<std::vector<double>> compressed_;  // сжатая матрица [display_size_][display_size_]
};



class HeatEquationSolver {
public:
    // Конструктор задаёт параметры задачи, но не запускает решение.
    HeatEquationSolver(int N, double epsilon, int max_iters)
        : N_(N), epsilon_(epsilon), max_iters_(max_iters), U_vec_(N * N), Unew_vec_(N * N)
    {}

    // Основной метод – выполняет инициализацию и запускает метод Якоби.
    // Возвращает {число итераций, достигнутая ошибка, время счёта}.
    // После вызова solve() данные готовы на хосте, и их можно получить через get_solution().
    std::tuple<int, double, double> solve() {
        initialize_grid_();
        auto result = solve_jacobi_();
        return result;
    }

    // Возвращает константный указатель на финальное поле (host, row-major).
    const double* get_solution() const {
        return U_vec_.data();
    }

private:
    int N_;
    double epsilon_;
    int max_iters_;
    const int check_interval_ = 50;

    std::vector<double> U_vec_;    // текущее приближение
    std::vector<double> Unew_vec_; // новое приближение

    // Инициализация сетки: углы + линейная интерполяция по границам.
    void initialize_grid_() {
        double* U = U_vec_.data();

        // Углы
        U[0] = 10.0;                            // (0,0) – нижний левый
        U[N_ - 1] = 20.0;                       // (0, N-1) – нижний правый
        U[(N_ - 1) * N_ + (N_ - 1)] = 30.0;     // (N-1, N-1) – верхний правый
        U[(N_ - 1) * N_] = 20.0;                // (N-1, 0) – верхний левый

        // Нижняя граница (i = 0)
        for (int j = 1; j < N_ - 1; ++j) {
            U[j] = 10.0 + (20.0 - 10.0) * j / (N_ - 1);
        }

        // Верхняя граница (i = N-1)
        for (int j = 1; j < N_ - 1; ++j) {
            U[(N_ - 1) * N_ + j] = 20.0 + (30.0 - 20.0) * j / (N_ - 1);
        }

        // Левая граница (j = 0)
        for (int i = 1; i < N_ - 1; ++i) {
            U[i * N_] = 10.0 + (20.0 - 10.0) * i / (N_ - 1);
        }

        // Правая граница (j = N-1)
        for (int i = 1; i < N_ - 1; ++i) {
            U[i * N_ + (N_ - 1)] = 20.0 + (30.0 - 20.0) * i / (N_ - 1);
        }

        // Внутренние точки – нули
        for (int i = 1; i < N_ - 1; ++i) {
            for (int j = 1; j < N_ - 1; ++j) {
                U[i * N_ + j] = 0.0;
            }
        }

        // Копируем начальное приближение в Unew
        std::copy(U_vec_.begin(), U_vec_.end(), Unew_vec_.begin());
    }

    // Решение уравнения теплопроводности методом Якоби с использованием OpenACC.
    // Возвращает число итераций, достигнутую ошибку и время работы (сек).
    std::tuple<int, double, double> solve_jacobi_() {
        double* U = U_vec_.data();
        double* Unew = Unew_vec_.data();

        int final_iter = 0;
        double final_error = 0.0;
        double elapsed_time = 0.0;

        #pragma acc data copy(U[0:N_*N_], Unew[0:N_*N_])
        {
            double max_error = 0.0;
            int iter = 0;
            auto start = std::chrono::high_resolution_clock::now();

            for (iter = 1; iter < max_iters_; ++iter) {
                // 1) Расчёт нового приближения (только внутренние точки)
                #pragma acc parallel loop collapse(2) present(U, Unew)
                for (int i = 1; i < N_ - 1; ++i) {
                    for (int j = 1; j < N_ - 1; ++j) {
                        Unew[i * N_ + j] = 0.25 * ( U[(i - 1) * N_ + j] +
                                                    U[(i + 1) * N_ + j] +
                                                    U[i * N_ + (j - 1)] +
                                                    U[i * N_ + (j + 1)] );
                    }
                }

                // 2) Вычисление максимальной ошибки  &&
                // 3) Копирование Unew -> U для следующей итерации
                if (iter % check_interval_ == 0) {
                    max_error = 0.0;
                    #pragma acc parallel loop collapse(2) present(U, Unew)  reduction(max:max_error)
                    for (int i = 1; i < N_ - 1; ++i) {
                        for (int j = 1; j < N_ - 1; ++j) {
                            auto k = i * N_ + j;
                            double diff = fabs(Unew[k] - U[k]);
                            // if (diff > max_error) max_error = diff;
                            max_error = fmax(max_error, diff);
                            U[k] = Unew[k];  // update
                        }
                    }
                    // 4) Проверка сходимости
                    if (max_error < epsilon_) break;
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            elapsed_time = std::chrono::duration<double>(end - start).count();

            #pragma acc update self(U[0:N_*N_])

            final_iter = iter;
            final_error = max_error;
        }

        return {final_iter, final_error, elapsed_time};
    }
};



// --------------------------------------------------------------------------
// Печать таблицы с результатами
void print_results_table(const std::vector<std::tuple<int, int, double, double>>& results) {
    std::cout << "\n+----------+-----------+---------------------+---------------------+\n"
              << "| Grid size| Iterations| Error               | Time (sec)          |\n"
              << "+----------+-----------+---------------------+---------------------+\n";
    for (const auto& [N, iters, err, time] : results) {
        std::cout << "| " << std::setw(8) << N << " | "
                  << std::setw(9) << iters << " | "
                  << std::setw(19) << std::scientific << std::setprecision(6) << err << " | "
                  << std::setw(19) << std::fixed << std::setprecision(6) << time << " |\n";
    }
    std::cout << "+----------+-----------+---------------------+---------------------+\n";
}


// --------------------------------------------------------------------------
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
        ("display-size,g", po::value<int>(&display_size)->default_value(15),
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

        // При необходимости можно создать объект MatrixPrinter:
        MatrixPrinter printer(solver.get_solution(), N, display_size);
        if (numeric_print)  printer.print_numeric();
        printer.print_color();
    }

    // print_results_table(results);

    return 0;
}
