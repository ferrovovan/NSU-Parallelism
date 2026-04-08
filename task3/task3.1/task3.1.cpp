#include <iostream>
#include <iomanip>   // setw, setprecision, fixed
#include <vector>
#include <utility>   // pair
#include <thread>
#include <chrono>


void matrix_vector_product_func(
    const std::vector<std::vector<int>>& matrix,
    const std::vector<int>& vector, std::vector<int>& result,
    int lb, int ub)  // lower, upper boundries
{
    for (int i = lb; i < ub; ++i) {
        result[i] = 0;
        for (int j = 0; j < matrix[i].size(); ++j)
            result[i] += matrix[i][j] * vector[j];
    }
}

void InitArray_func(std::vector<int>& array, int start, int end) {
    for (int i = start; i < end; ++i)
        array[i] = i;
}

double run_parallel(size_t M, int num_of_threads)
{   // N = M

    // --- Инициализация ---
    std::vector<std::vector<int>> matrix(M, std::vector<int>(M, 1));
    std::vector<int> vector(M, 2);
    std::vector<int> result(M);

    std::vector<std::thread> threads;  // контейнер для объектов потоков
    int range_size = M / num_of_threads;  // сколько строк возьмет на себя один поток

    for (int k = 0; k < num_of_threads; ++k)  // параллельная инициализация, добавляя объекты потоков в конец вектора
        threads.emplace_back(InitArray_func, std::ref(vector), k * range_size, (k + 1) * range_size);
        // не threads.push_back, т.к. push_back _перемещает_, а emplace_back сразу в память кладёт.
    for (auto& thread : threads)
        thread.join();  // ожидание завершения потока


    // --- Основная работа ---
    auto start_time = std::chrono::high_resolution_clock::now();  // время старта

    for (int k = 0; k < num_of_threads; ++k)  // каждый поток вызывает функцию matrix_vector_product для своего диапазона значений
        threads[k] = std::thread(matrix_vector_product_func, std::ref(matrix), std::ref(vector), std::ref(result), k * range_size, (k + 1) * range_size);
    for (auto& thread : threads)
        thread.join();  // ожидание завершения потока

    auto end_time   = std::chrono::high_resolution_clock::now();  // время финиша

    double runtime = std::chrono::duration<double>(end_time - start_time).count();  // время работы на num_threads[i] потоках
    return runtime;
}


void print_table(const std::vector<std::pair<int, double>>& results)
{
    double serial_time = results.front().second;
    std::cout << "\nElapsed serial time: "
                << std::fixed << std::setprecision(6) << serial_time << " sec\n";

    // Заголовок таблицы
    std::cout << "\n+----------+---------------------+----------+\n"
                << "| Threads  | Elapsed time (sec)  | Speedup  |\n"
                << "+----------+---------------------+----------+\n";

    for (const auto& [threads, runtime] : results) {
        double speedup = serial_time / runtime;
        std::cout << "| " << std::setw(8) << threads << " | "
                    << std::setw(19) << std::fixed << std::setprecision(6) << runtime << " | "
                    << std::setw(8) << std::fixed << std::setprecision(3) << speedup << " |\n";
    }

    std::cout << "+----------+---------------------+----------+\n";
}


int main(int argc, char* argv[])
{
    /* ---------- Считывание аргументов ---------- */
    size_t M = 20000;
    // size_t N = M;

    if (argc > 1)
        M = atoi(argv[1]);
    std::cout << "Input size:\tM=" << M << ";\n";

    std::vector<int> list_num_threads = {1, 2, 4, 7, 8, 16, 20, 40};
    std::vector<std::pair<int, double>> runtimes;
    runtimes.reserve(list_num_threads.size());

    for (int num_threads : list_num_threads) {
        double t = run_parallel(M, num_threads);
        runtimes.emplace_back(num_threads, t);
   }

    print_table(runtimes);

    return 0;
}
