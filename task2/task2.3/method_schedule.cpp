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

    // Вычисляем норму правой части
    double b_norm_sq = 0.0;
    for (int i = 0; i < N; ++i) {
        b_norm_sq += b[i] * b[i];
    }
    double b_norm = std::sqrt(b_norm_sq);

    double loss_prev = INT_MAX;
    double tau = TAU;
    double err = 1;
    int chunk = N / threads_count;
    if (chunk < 1) chunk = 1; // Страховка, если потоков больше, чем строк


    while (err > EPS) {
        double norm_sq = 0.0;
        std::vector<double> x_new(N, 0.0);

        #pragma omp parallel for schedule(static, chunk) num_threads(threads_count) reduction(+:norm_sq)  // оставляем часть не занятыми, чтобы по готовности поток брал
        for (int i = 0; i < N; i++) {
            double sum = 0;
            for (int j = 0; j < N; j++)
                sum += A[i * N + j] * x[j];
            double r = sum - b[i]; // невязка
            norm_sq += r * r;

            x_new[i] = x[i] - tau * r;  // обновление решения
        }

        err = std::sqrt(norm_sq) / b_norm;
        if ((loss_prev - err) < 1e-4)  tau *= -1;
        loss_prev = err;

        x = std::move(x_new);
    }
    return x;
}
