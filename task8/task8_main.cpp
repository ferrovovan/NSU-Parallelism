#include <iostream>
#include <stdbool.h>
#include <vector>
#include <string>
#include <tuple>

#include <boost/program_options.hpp>
#include "jacobi_solver.cuh"


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
        : N_(N), epsilon_(epsilon), max_iters_(max_iters), U_vec_(N * N)  {}

    // Основной метод – выполняет инициализацию и запускает метод Якоби.
    // Возвращает {число итераций, достигнутая ошибка, время счёта}.
    // После вызова solve() данные готовы на хосте, и их можно получить через get_solution().
    std::tuple<int, double, double> solve() {
        initialize_grid_();
        auto [iters, error, time] = solve_jacobi_();
        return {iters, error, time};
    }

    const double* get_solution() const {
        return U_vec_.data();
    }

private:
    int N_;
    double epsilon_;
    int max_iters_;
    std::vector<double> U_vec_;

    // Угловые значения
    const double lower_left_value  = 10.0;
    const double lower_right_value = 20.0;
    const double upper_left_value  = 20.0;
    const double upper_right_value = 35.0;

    void initialize_grid_() {
        double* U = U_vec_.data();

        // Углы
        U[0] = lower_left_value;                             // (0,0) – нижний левый
        U[N_ - 1] = lower_right_value;                       // (0, N-1) – нижний правый
        U[(N_ - 1) * N_ + (N_ - 1)] = upper_right_value;     // (N-1, N-1) – верхний правый
        U[(N_ - 1) * N_] = upper_left_value;                 // (N-1, 0) – верхний левый

        // Нижняя граница (i = 0, j = 1..N-2) 10 -> 20
        for (int j = 1; j < N_ - 1; ++j) {
            U[j] = lower_left_value + (lower_right_value - lower_left_value) * (j + 1) / (N_ - 1);
        }

        // Верхняя граница (i = N-1, j = 1..N-2) 20 -> 30
        for (int j = 1; j < N_ - 1; ++j) {
             U[(N_ - 1) * N_ + j] = upper_left_value + (upper_right_value - upper_left_value) * (j + 1) / (N_ - 1);
        }

        // Левая граница (j = 0, i = 1..N-2) 10 -> 20
        for (int i = 1; i < N_ - 1; ++i) {
            U[i * N_] = lower_left_value + (upper_left_value - lower_left_value) * (i + 1) / (N_ - 1);
        }

        // Правая граница (j = N-1, i = 1..N-2) 20 -> 30
        for (int i = 1; i < N_ - 1; ++i) {
            U[i * N_ + (N_ - 1)] = lower_right_value + (upper_right_value - lower_right_value) * (i + 1) / (N_ - 1);
        }

        // Внутренние точки – нули
        for (int i = 1; i < N_ - 1; ++i) {
            for (int j = 1; j < N_ - 1; ++j) {
                U[i * N_ + j] = 0.0;
            }
        }
    }

    // Основной итерационный метод
    std::tuple<int, double, double> solve_jacobi_() {
        auto [iters, error, elapsed_time] = solve_jacobi_cuda(N_, epsilon_, max_iters_,
                              U_vec_.data());
        return {iters, error, elapsed_time};
    }
};


int main(int argc, char* argv[]) {
    namespace po = boost::program_options;

    // Объявление параметров командной строки
    std::vector<int> grid_sizes;
    double epsilon = 1e-4;
    int max_iterations = 100000;
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
