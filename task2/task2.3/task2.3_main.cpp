#include "config.hpp"
#include "method.hpp"  // simple_iteration_method
#include <iostream>
#include <vector>
#include <omp.h>     // для omp_get_wtime
#include <cstdlib>   // для atoi
#include <cstdio>    // для printf


int main(int argc, char* argv[])
{
    std::ios::sync_with_stdio(true); // для смешивания cout и printf. Включено по-умолчанию

    /* ---------- Считывание аргументов ---------- */
    int threads_list[8] = {1,2,4,7,8,16,20,40};
    int last_index = 8; // до какого индекса проверять, по-умолчанию
    if (argc > 1) {
        int input_index = atoi(argv[1]);
        if (0 < input_index && input_index <= 8)
            last_index = input_index;
        else {
            printf("input_index {%d} out of range (1-8)\n", input_index);
            return 1;
        }
    }
    int max_threads = threads_list[last_index - 1];


    /* ---------- Инициализация ---------- */
    // матрица A
    std::vector<double> A(N * N, 1.0);
    #pragma omp parallel for num_threads(max_threads)
    for (int i = 0; i < N; i++)  // карусельная инициализация
        A[i * N + i] = 2.0;
    // вектор b
    std::vector<double> b(N, 1.0+N);   // все компоненты равны 1+N


    /* ---------- Выполнение ---------- */
    std::vector<double> parallel_times(last_index);
    for (int idx = 2; idx < last_index; ++idx) {
        int thr = threads_list[idx];
        double start_time = omp_get_wtime();
        std::vector<double> solution = simple_iteration_method(A, b, thr);
        double elapsed = omp_get_wtime() - start_time;
        parallel_times[idx] = elapsed;

        std::cout << "Threads = " << thr << ": time = " << elapsed << " s\n";
        std::cout << "  solution[0-2:-2,-1] = ";
        for (int i = 0; i <= 2; ++i)
            std::cout << solution[i] << " ";
        std::cout << " ... ";
        for (int i = N-2; i < N; ++i)
            std::cout << solution.at(i) << " ";
        std::cout << std::endl;
    }

    // ---------- Таблица результатов ----------
    std::cout << "\n+----------+---------------------+\n";
    std::cout << "| Threads  | Elapsed time (sec)  |\n";
    std::cout << "+----------+---------------------+\n";
    for (int idx = 0; idx < last_index; ++idx) {
        printf("| %8d | %19.6f |\n", threads_list[idx], parallel_times[idx]);
    }
    std::cout << "+----------+---------------------+\n";

    return 0;
}


/* Вариант с выбором наименьшего времени */
// int main(int argc, char* argv[]) {
//     int cnt[8] = {1,2,4,7,8,16,20,40};
//     double result[8] = {40, 40, 40, 40, 40, 40, 40, 40};
//     std::vector<double> A(n * n, 1.0);

//     for(int g = 0; g < 3; g++){
//         for(int k = 0; k < 8; k++){

//             #pragma omp parallel for num_threads(cnt[k])
//             for (int i = 0; i < n; i++)
//                 A[i * n + i] = 2.0;
//             std::vector<double> b(n, 1 + n);

//             double t1 = omp_get_wtime();
//             std::vector<double> solution = simple_iteration_method(A, b, cnt[k]);
//             t1 = omp_get_wtime() - t1;

//             // std::cout << "Решение системы:" << std::endl;
//             // for (int i = 0; i < std::min(10, n); ++i)
//             //         std::cout << "x[" << i << "] = " << solution[i] << std::endl;
//             if(t1 < result[k])
//                 result[k] = t1;

//             // printf("%.6f\n", t1);
//         }
//         printf(">>>>iteration:%d\n", g);
//     }

//     for(int i = 0; i < 8; i++){
//         printf("%d threads: \n", cnt[i]);
//         printf("%.6f\n", result[i]);
//     }

//     return 0;
// }
