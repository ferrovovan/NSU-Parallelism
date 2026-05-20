#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

// Функция для измерения времени (секунды)
double cpuSecond()
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ((double)ts.tv_sec + (double)ts.tv_nsec * 1.e-9);
}


/* Программы  множения матрицы на вектор */

// Последовательное умножение матрицы на вектор
void matrix_vector_product(double *a, double *b, double *c, size_t m, size_t n)
{
    for (size_t i = 0; i < m; i++)
    {
        c[i] = 0.0;
        for (size_t j = 0; j < n; j++)
            c[i] += a[i * n + j] * b[j];
    }
}

// Параллельное умножение матрицы на вектор (ручное распределение итераций)
void matrix_vector_product_omp(double* a, double* b, double* c, size_t m, size_t n, int num_of_threads)
{
    #pragma omp parallel num_threads(num_of_threads)
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();

        size_t items_per_thread = m / nthreads;
        size_t lb = threadid * items_per_thread;
        size_t ub = (threadid == nthreads - 1) ? (m - 1) : (lb + items_per_thread - 1);

        for (size_t i = lb; i <= ub; i++) {
            c[i] = 0.0;
            for (size_t j = 0; j < n; j++)
                c[i] += a[i * n + j] * b[j];
        }
    }
}


/* Применение программ */

void safe_varibles_malloc
(double **a, double **b, double **c, size_t n, size_t m)
{
    *a = (double*)malloc(sizeof(double) * m * n);
    *b = (double*)malloc(sizeof(double) * n);
    *c = (double*)malloc(sizeof(double) * m);

    if (*a == NULL || *b == NULL || *c == NULL)
    {
        free(*a); free(*b); free(*c);
        printf("Error allocate memory!\n");
        exit(1);
    }
}

// Последовательная версия: выделение памяти, инициализация, замер времени
double run_serial(size_t n, size_t m)
{
    double *a, *b, *c;
    safe_varibles_malloc(&a, &b, &c, n, m);

    // Инициализация матрицы и вектора
    for (size_t i = 0; i < m; i++)
        for (size_t j = 0; j < n; j++)
            a[i * n + j] = i + j;

    for (size_t j = 0; j < n; j++)
        b[j] = j;

    // Замер времени
    double t = cpuSecond();
    matrix_vector_product(a, b, c, m, n);
    t = cpuSecond() - t;

    free(a); free(b); free(c);
    return t;
}

// Параллельная версия: инициализация и умножение распараллелены
double run_parallel(size_t n, size_t m, int num_of_threads)
{
    // Объявление
    double *a, *b, *c;
    safe_varibles_malloc(&a, &b, &c, n, m);

    /* Инициализация матрицы и вектора */
    // Параллельная инициализация матрицы A
    #pragma omp parallel num_threads(num_of_threads)
    {
        int nthreads = omp_get_num_threads();       // кол-во потоков
        int threadid = omp_get_thread_num();        // id/номер потока
        size_t items_per_thread = m / nthreads;     // кол-во итераций за один поток
        size_t lb = threadid * items_per_thread;    // нижная граница
        size_t ub = (threadid == nthreads - 1) ? (m - 1) : (lb + items_per_thread - 1); // верхняя граница

        for (size_t i = lb; i <= ub; i++)
            for (size_t j = 0; j < n; j++)
                a[i * n + j] = i + j;
    }

    // Параллельная инициализация вектора B
    #pragma omp parallel num_threads(num_of_threads)
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        size_t items_per_thread = n / nthreads;
        size_t lb = threadid * items_per_thread;
        size_t ub = (threadid == nthreads - 1) ? (n - 1) : (lb + items_per_thread - 1);

        for (size_t j = lb; j <= ub; j++)
            b[j] = j;
    }

    // Замер времени
    double t = cpuSecond();
    matrix_vector_product_omp(a, b, c, m, n, num_of_threads);
    t = cpuSecond() - t;

    free(a); free(b); free(c);
    return t;
}


/* Функции для вывода отчёта */

void print_table(int *thread_counts, double *times, int size, double serial_time)
{
    printf("\nElapsed serial time: %.6f sec\n", serial_time);
    printf("\n+----------+---------------------+----------+\n");
    printf("| Threads  | Elapsed time (sec)  | Speedup  |\n");
    printf("+----------+---------------------+----------+\n");

    for (int i = 0; i < size; i++) {
        double speedup = serial_time / times[i];
        printf("| %8d | %19.6f | %8.3f |\n", thread_counts[i], times[i], speedup);
    }

    printf("+----------+---------------------+----------+\n");
}

int write_csv_data(char *file_name,
    int *thread_counts, double *times, int size, double serial_time)
{
    FILE *file = fopen(file_name, "w");
    if (file == NULL)  return 1;

    fprintf(file, "Elapsed time (sec);Speedup\n");
    for (int i = 0; i < size; i++) {
        double speedup = serial_time / times[i];
        fprintf(file, "%.6f;%.3f;\n", times[i], speedup);
    }
    fclose(file);
    return 0;
}



int main(int argc, char* argv[])
{
    /* ---------- Считывание аргументов ---------- */
    size_t M = 20000;
    size_t N = M;

    int cnt[8] = {1, 2, 4, 7, 8, 16, 20, 40};  // согласно заданию, проверочное число потоков
    int last_index = 8;  // кол-во первых аргументов (числа из массива потоков) для применения

    if (argc > 1)
        M = atoi(argv[1]);
    if (argc > 2)
        N = atoi(argv[2]);
    if (argc > 3) {  // Для выполнения лишь первых значений
        int input_index = atoi(argv[3]);
        if (0 < input_index && input_index < 8)
            last_index = input_index;
        else {
            printf("input_index {%d} out of range (1-8)\n", input_index);
            return 1;
        }
    }


    double serial_time;
    double parallel_times[last_index];

    printf("Input size:\tM=%zu;\n", M);
    printf("           \tN=%zu;\n", N);
    serial_time = run_serial(M, N);

    for (int i = 0; i < last_index; i++) {
        parallel_times[i] = run_parallel(M, N, cnt[i]);
    }

    print_table(cnt, parallel_times, last_index, serial_time);
    write_csv_data("task1.csv",
        cnt, parallel_times, last_index, serial_time);

    return 0;
}
