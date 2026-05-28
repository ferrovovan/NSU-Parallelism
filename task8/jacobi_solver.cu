#include "jacobi_solver.cuh"
#include <cuda_runtime.h>
#include <cub/cub.cuh>
#include <cmath>
#include <chrono>

#define BLOCK_REDUCE 256   // размер блока для редукции CUB
#define FINAL_REDUCE 256   // размер блока финальной редукции

#define IDX(i, j, size) ((long long)(i) * (size) + (j))


// Ядро одного шага метода Якоби: вычисление нового un, ошибки diff и копирование в u
__global__ void jacobi_update(double* u, double* un, double* diff, int size) {
    int i = blockIdx.y * blockDim.y + threadIdx.y + 1;
    int j = blockIdx.x * blockDim.x + threadIdx.x + 1;
    if (i >= size - 1 || j >= size - 1) return;

    int idx = IDX(i,j,size);
    double new_val = 0.25 * (u[IDX(i,j-1,size)] + u[IDX(i,j+1,size)] +
                             u[IDX(i-1,j,size)] + u[IDX(i+1,j,size)]);
    double old = u[idx];
    diff[idx] = fabs(new_val - old);
    un[idx] = new_val;
    u[idx] = new_val;
}

// Блочная редукция: каждый блок находит локальный максимум своего сегмента
template <int BLOCK_SIZE>
__global__ void block_max_kernel(const double* diff, double* block_max, int n) {
    typedef cub::BlockReduce<double, BLOCK_SIZE> BlockReduce;
    __shared__ typename BlockReduce::TempStorage temp_storage;

    double thread_max = 0.0;
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    // Цикл по всем элементам, закреплённым за блоком
    for (int i = idx; i < n; i += blockDim.x * gridDim.x) {
        double val = diff[i];
        if (val > thread_max) thread_max = val;
    }

    double block_result = BlockReduce(temp_storage).Reduce(thread_max, cub::Max());
    if (threadIdx.x == 0) {
        block_max[blockIdx.x] = block_result;
    }
}

// Финальная редукция: один блок сворачивает массив локальных максимумов
template <int BLOCK_SIZE>
__global__ void final_max_kernel(const double* block_max, int len, double* global_max) {
    typedef cub::BlockReduce<double, BLOCK_SIZE> BlockReduce;
    __shared__ typename BlockReduce::TempStorage temp_storage;

    double thread_max = 0.0;
    int idx = threadIdx.x;
    for (int i = idx; i < len; i += blockDim.x) {
        double val = block_max[i];
        if (val > thread_max) thread_max = val;
    }

    double result = BlockReduce(temp_storage).Reduce(thread_max, cub::Max());
    if (threadIdx.x == 0) {
        *global_max = result;
    }
}

// Основной алгоритм-связка
std::tuple<int, double, double> solve_jacobi_cuda(
    int N, double eps, int max_iter,  // вход для алгоритма
    double *h_u)  // нам посмотреть итоги
{
    int size = N;
    long long n = (long long)size * size;

    // --- Выделение памяти на устройстве ---
    double *d_u, *d_un, *d_diff;
    cudaMalloc(&d_u, n * sizeof(double));
    cudaMalloc(&d_un, n * sizeof(double));
    cudaMalloc(&d_diff, n * sizeof(double));
    cudaMemcpy(d_u, h_u, n * sizeof(double), cudaMemcpyHostToDevice);
    // cudaMemcpy(d_un, h_un, n * sizeof(double), cudaMemcpyHostToDevice);
    cudaMemset(d_un  , 0, n * sizeof(double));
    cudaMemset(d_diff, 0, n * sizeof(double));

    // Память для результатов блочной редукции и глобального максимума
    int block_reduce_grid = (n + BLOCK_REDUCE - 1) / BLOCK_REDUCE;
    double *d_block_max, *d_global_max;
    cudaMalloc(&d_block_max, block_reduce_grid * sizeof(double));
    cudaMalloc(&d_global_max, sizeof(double));

    // Pinned host-память для асинхронного чтения ошибки из графа
    double* h_error_pinned;
    cudaHostAlloc(&h_error_pinned, sizeof(double), cudaHostAllocDefault);

    // --- Настройка параметров запуска ядер ---
    dim3 jacobi_block(16, 16);
    dim3 jacobi_grid((N + jacobi_block.x - 1) / jacobi_block.x,
                     (N + jacobi_block.y - 1) / jacobi_block.y);

    // --- Создание и захват CUDA Graph ---
    cudaStream_t stream;
    cudaStreamCreate(&stream);

    cudaGraph_t graph;
    cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal);

    // 1. Шаг Якоби + вычисление разностей
    jacobi_update<<<jacobi_grid, jacobi_block, 0, stream>>>(d_u, d_un, d_diff, size);

    // 2. Блочная редукция (локальные максимумы)
    block_max_kernel<BLOCK_REDUCE><<<block_reduce_grid, BLOCK_REDUCE, 0, stream>>>(
        d_diff, d_block_max, n);

    // 3. Финальная редукция (один блок)
    final_max_kernel<FINAL_REDUCE><<<1, FINAL_REDUCE, 0, stream>>>(
        d_block_max, block_reduce_grid, d_global_max);

    // 4. Асинхронное копирование глобального максимума на хост
    cudaMemcpyAsync(h_error_pinned, d_global_max, sizeof(double),
                    cudaMemcpyDeviceToHost, stream);

    cudaStreamEndCapture(stream, &graph);
    cudaGraphExec_t graphExec;
    cudaGraphInstantiate(&graphExec, graph, NULL, NULL, 0);

    // --- Итерационный цикл с запуском графа ---
    int iter = 1;
    double error = 1.0;
    auto start = std::chrono::high_resolution_clock::now();
    int check_interval_ = 10;
    if (100 < max_iter)   check_interval_ = 100;
    if (1000 < max_iter)  check_interval_ = 1000;
    // if (10000 < max_iter) check_interval_ = 5000;

    while (iter < max_iter) {
        cudaGraphLaunch(graphExec, stream);
        cudaStreamSynchronize(stream);

        error = *h_error_pinned;
        if (error < eps)
            break;
        ++iter;
        if (iter % check_interval_ == 0) {
            std::cout << "Iter " << iter
                      << "\terror: " << error
                      << "\n";
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_time = std::chrono::duration<double>(end - start).count();

    // Отдаём решение сетки
    cudaMemcpy(h_u, d_u, n * sizeof(double), cudaMemcpyDeviceToHost);

    // --- Очистка ---
    cudaGraphExecDestroy(graphExec);
    cudaGraphDestroy(graph);
    cudaStreamDestroy(stream);
    cudaFree(h_error_pinned);
    cudaFree(d_u);
    cudaFree(d_un);
    cudaFree(d_diff);
    cudaFree(d_block_max);
    cudaFree(d_global_max);

    return {iter, error, elapsed_time};
}
