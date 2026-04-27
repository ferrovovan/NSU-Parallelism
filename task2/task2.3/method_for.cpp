#include "method.hpp"
#include "config.hpp"
#include <vector>
#include <omp.h>
#include <cmath>     // для sqrt


/**
 * Вычисляет вектор невязки r = A*x - b и его евклидову норму.
 *
 * @param A    матрица размером N×N (хранится по строкам)
 * @param b    вектор правой части длины N
 * @param x    текущее приближение решения длины N
 * @param r    выходной вектор невязки (должен быть размера N)
 * @param threads количество потоков OpenMP
 * @return     евклидова норма вектора r
 */
double compute_residual(const std::vector<double>& A, const std::vector<double>& b,
                        const std::vector<double>& x, std::vector<double>& r,
                        int threads_count)
{
    double norm_sq = 0.0;  // сумма квадратов компонент невязки

    #pragma omp parallel for num_threads(threads_count) reduction(+:norm_sq)
    for (int i = 0; i < N; ++i) {
        double sum = 0.0;
        // Умножение i-й строки матрицы A на вектор x
        for (int j = 0; j < N; ++j) {
            sum += A[i * N + j] * x[j];
        }
        r[i] = sum - b[i];
        /* благодаря reduction(+:norm_sq) не нужно ставить #pragma omp atomic */
        norm_sq += r[i] * r[i];
    }
    return std::sqrt(norm_sq);
}

/**
 * Обновляет приближение решения по формуле x_new = x - τ * r.
 *
 * @param x      текущее приближение
 * @param r      вектор невязки
 * @param tau    параметр метода (τ)
 * @param x_new  выходной новый вектор (должен быть размера N)
 * @param threads количество потоков
 */
void update_solution(const std::vector<double>& x, const std::vector<double>& r,
                     double tau, std::vector<double>& x_new, int threads_count)
{
    #pragma omp parallel for num_threads(threads_count)
    for (int i = 0; i < N; ++i) {
        x_new[i] = x[i] - tau * r[i];
    }
}

/**
 * Метод простой итерации (метод Ричардсона) для решения СЛАУ Ax = b.
 *
 * @param A    матрица системы (N×N, плотная)
 * @param b    вектор правой части
 * @param threads количество потоков для параллельных участков
 * @return     приближённое решение x
 */
std::vector<double> simple_iteration_method(
    const std::vector<double>& A,
    const std::vector<double>& b,
    int threads_count)
{
    // Начальное приближение – нулевой вектор
    std::vector<double> x(N, 0.0);
    std::vector<double> x_new(N);
    std::vector<double> r(N);

    // Вычисляем норму правой части (один раз)
    double b_norm_sq = 0.0;
    for (int i = 0; i < N; ++i) {
        b_norm_sq += b[i] * b[i];
    }
    double b_norm = std::sqrt(b_norm_sq);

    /* Итерационный процесс */
    while (true) {
        // 1. Вычисляем невязку и её норму
        double r_norm = compute_residual(A, b, x, r, threads_count);

        // 2. Проверка условия остановки: ||τ·r|| / ||b|| < EPS
        if (TAU * r_norm / b_norm  < EPS) {
            break;
        }

        // 3. Строим новое приближение
        update_solution(x, r, TAU, x_new, threads_count);

        // 4. Переключаем векторы (старое приближение больше не нужно)
        x.swap(x_new);
    }

    return x;
}
