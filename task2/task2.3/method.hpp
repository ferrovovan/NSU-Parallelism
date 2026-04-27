#ifndef METHOD_HPP
#define METHOD_HPP

#include <vector>

std::vector<double> simple_iteration_method(
    const std::vector<double>& A,
    const std::vector<double>& b,
    int threads_count);

#endif // METHOD_HPP
