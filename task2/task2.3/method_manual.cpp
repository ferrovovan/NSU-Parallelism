// #include <iostream>   // cout endl
#include "method.hpp"
#include "config.hpp"
#include <vector>
#include <omp.h>
#include <cmath>     // для sqrt
#include <limits.h>


std::vector<double> simple_iteration_method(
    const std::vector<double> &A,
    const std::vector<double> &b,
    int threads_count)
{
    std::vector<double> x(N, 0.0);
    std::vector<double> x_new(N, 0.0);

    // Вычисляем норму правой части
    double b_norm_sq = 0.0;
    for (int i = 0; i < N; ++i) {
        b_norm_sq += b[i] * b[i];
    }
    double b_norm = std::sqrt(b_norm_sq);

    double loss_prev = INT_MAX;
    double tau = TAU;
    double err = 1;
    double norm_sq = 0.0;  // сумма квадратов компонент невязки


    while (err > EPS) {
        norm_sq = 0;
// std::cout << "Err = " << err << ";TAU = " << tau << std::endl;  // Отладка (на разбалтывание)
        #pragma omp parallel num_threads(threads_count) reduction(+:norm_sq)
        { // Сами делаем работу omp parallel for (+ охватываем проверку)
            /* Распределение работы */
            int nthreads = omp_get_num_threads();
            int tid = omp_get_thread_num();  // threadid

            // 1. Считаем диапазон для конкретного потока
            int items_per_thread = N / nthreads;
            int lb = tid * items_per_thread;
            int ub = (tid == nthreads - 1) ? (N - 1) : (lb + items_per_thread - 1);
            /* Вычисление невязки + обновление приближения */
            for (int i = lb; i <= ub; i++) {
                double sum = 0;
                for (int j = 0; j < N; j++)
                    sum += A[i * N + j] * x[j];
                double r = sum - b[i]; // невязка
                norm_sq += r * r;  // pow(r, 2) медленнее, чем r * r

                x_new[i] = x[i] - tau * r;  // обновление решения
            }

            // Однопоточное обновление (барьер после цикла НЕ сработает сам)
            #pragma omp barrier
            #pragma omp single  // выполняется на одном потоке, т.к. вычисления одинаковые
            {
                err = std::sqrt(norm_sq) / b_norm;
                if ((loss_prev - err) < TAU)  tau *= -1;
                loss_prev = err;

                // x = std::move(x_new);  // `x = x_new` работает за O(n)
                x.swap(x_new);
                // x = x_new;
                norm_sq = 0;
            }
            // Здесь неявный барьер в конце single — все ждут обновления X
            #pragma omp barrier
        }
        // std::cout << "\033[F" // Перейти к предыдущей строке
		          // << "\033[K"; // Очистить строку
    }
    return x;
}
