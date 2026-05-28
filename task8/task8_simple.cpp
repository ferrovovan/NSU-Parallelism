#include <iostream>
#include <cstring>
#include <cstdlib>
#include "jacobi_solver.cuh"

#define IDX(i, j, size) ((long long)(i) * (size) + (j))

double* init_grid(int N) {
    int size = N;
    long long n = (long long)size * size;
    double *u = new double[n]();    // сразу нули

    u[IDX(0, 0, size)] = 10.0;
    u[IDX(0, size-1, size)] = 20.0;
    u[IDX(size-1, 0, size)] = 30.0;
    u[IDX(size-1, size-1, size)] = 20.0;

    for (int j = 1; j < size - 1; j++) {
        double t = (double)j / (size - 1);
        u[IDX(0, j, size)] = 10.0 + 10.0 * t;
        u[IDX(size-1, j, size)] = 30.0 - 10.0 * t;
    }
    for (int i = 1; i < size - 1; i++) {
        double t = (double)i / (size - 1);
        u[IDX(i, 0, size)] = 10.0 + 20.0 * t;
        u[IDX(i, size-1, size)] = 20.0;
    }
    return u;
}

int main(int argc, char* argv[]) {
    // --- Параметры по умолчанию ---
    int N = 128;
    double eps = 1e-4;
    int max_iter = 100000;

    // --- Разбор аргументов командной строки ---
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            N = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--eps") == 0 && i + 1 < argc) {
            eps = atof(argv[++i]);
        } else if (strcmp(argv[i], "--max_iter") == 0 && i + 1 < argc) {
            max_iter = atoi(argv[++i]);
        }
    }

    double* U = init_grid(N);
    // Основной вызов
    auto [iters, error, time] = solve_jacobi_cuda(N, eps, max_iter, U);

    // Вывод по мелочи
    std::cout << "Размер: " << N << "x" << N << std::endl;
    std::cout << "Время: " << time << " мс" << std::endl;
    std::cout << "Итерации: " << iters << std::endl;
    std::cout << "Ошибка: " << error << std::endl;

    delete[] U;
    return 0;
}
