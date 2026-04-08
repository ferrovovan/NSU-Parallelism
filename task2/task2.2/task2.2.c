#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <math.h>

const double PI = 3.14159265358979323846;
const double a = -4.0;
const double b = 4.0;
const int nsteps = 40000000;  // nsteps - число точек интегрирования

double cpuSecond()
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ((double)ts.tv_sec + (double)ts.tv_nsec * 1.e-9);
}


// Численное интегрирование (метод прямоугольников)
double func(double x) {
    return exp(-x * x);
}

double integrate(double (*func)(double), double a, double b, int n)
{
    double h = (b - a) / n;
    double sum = 0.0;

    for (int i = 0; i < n; i++)
        sum += func(a + h * (i + 0.5));

    sum *= h;

    return sum;
}

// Параллельная версия
double integrate_omp(double (*func)(double), double a, double b, int n, int count)
{
    double h = (b - a) / n;
    double sum = 0.0;

    #pragma omp parallel num_threads(count)
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int items_per_thread = n / nthreads;
        int lb = threadid * items_per_thread;  // lower bound
        int ub = (threadid == nthreads - 1) ? (n - 1) : (lb + items_per_thread - 1);
        double sumloc = 0.0;

        for (int i = lb; i <= ub; i++)
            sumloc += func(a + h * (i + 0.5));

        #pragma omp atomic
        sum += sumloc;
    }
    sum *= h;
    return sum;
}


double run_serial()
{
    double t = cpuSecond();
    //double res = integrate(func, a, b, nsteps);
    integrate(func, a, b, nsteps);
    t = cpuSecond() - t;
    //printf("Result (serial): %.12f; error %.12f\n", res, fabs(res - sqrt(PI)));
    return t;
}

double run_parallel(int count)
{
    double t = cpuSecond();
    //double res = integrate_omp(func, a, b, nsteps, count);
    integrate_omp(func, a, b, nsteps, count);
    t = cpuSecond() - t;
    //printf("Result (parallel): %.12f; error %.12f\n", res, fabs(res - sqrt(PI)));
    return t;
}


// Функция для вывода красивой таблицы
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

    fprintf(file, "Threads;Elapsed time (sec);Speedup\n");
    for (int i = 0; i < size; i++) {
        double speedup = serial_time / times[i];
        fprintf(file, "%d;%.6f;%.3f;\n",
            thread_counts[i], times[i], speedup);
    }
    fclose(file);
    printf("\nWrote data in %s\n", file_name);
    return 0;
}



int main(int argc, char **argv)
{
    /* ---------- Считывание аргументов ---------- */
    int cnt[8] = {1, 2, 4, 7, 8, 16, 20, 40};  // согласно заданию
    int last_index = 8;

    if (argc > 1) {  // Для выполнения лишь первых значений
        int input_index = atoi(argv[3]);
        if (0 < input_index && input_index <= 8)
            last_index = input_index;
        else {
            printf("input_index {%d} out of range (1-8)\n", input_index);
            return 1;
        }
    }


    double serial_time;
    double parallel_times[last_index];

    printf("Integration f(x) on: [%.12f, %.12f],\n\tnsteps = %d\n", a, b, nsteps);
    serial_time = run_serial();

    for (int i = 0; i < last_index; i++) {
        parallel_times[i] = run_parallel(cnt[i]);
    }

    print_table(cnt, parallel_times, last_index, serial_time);
    write_csv_data("task2.csv",
        cnt, parallel_times, last_index, serial_time);

    // Result (parallel): 1.772453823579; error 0.000000027326
    // Result (serial):   1.772453823579; error 0.000000027326
    return 0;
}
