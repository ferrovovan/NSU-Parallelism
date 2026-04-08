#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>


const double EPSILON = 1e-2;

// Helper function to trim whitespace
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

// Parse line for sin and sqrt: "func(arg) = result"
bool parseFuncLine(const std::string& line, double& arg, double& expected) {
    size_t open = line.find('(');
    size_t close = line.find(')');
    size_t eq = line.find('=');
    if (open == std::string::npos || close == std::string::npos || eq == std::string::npos)
        return false;

    std::string argStr = line.substr(open + 1, close - open - 1);
    std::string expStr = line.substr(eq + 1);
    try {
        arg = std::stod(trim(argStr));
        expected = std::stod(trim(expStr));
    } catch (...) {
        return false;
    }
    return true;
}

// Parse line for pow: "base^2 = result"
bool parsePowLine(const std::string& line, double& arg, double& expected) {
    size_t caret = line.find('^');
    size_t eq = line.find('=');
    if (caret == std::string::npos || eq == std::string::npos)
        return false;

    std::string argStr = line.substr(0, caret);
    std::string expStr = line.substr(eq + 1);
    try {
        arg = std::stod(trim(argStr));
        expected = std::stod(trim(expStr));
    } catch (...) {
        return false;
    }
    return true;
}

// Generic checker
template<typename Func>
bool checkFile(const std::string& filename, const std::string& funcName, Func compute,
               bool (*parseLine)(const std::string&, double&, double&)) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: cannot open file " << filename << std::endl;
        return false;
    }

    std::string line;
    int lineNum = 0;
    int errors = 0;

    while (std::getline(file, line)) {
        lineNum++;
        if (line.empty()) continue;

        double arg, expected;
        if (!parseLine(line, arg, expected)) {
            std::cerr << "Parse error in " << filename << " line " << lineNum << ": " << line << std::endl;
            errors++;
            continue;
        }

        double computed = compute(arg);
        if (std::fabs(computed - expected) > EPSILON) {
            std::cerr << std::fixed << std::setprecision(6);
            std::cerr << "Mismatch in " << filename << " line " << lineNum << ": "
                      << funcName << "(" << arg << ") = " << computed
                      << ", expected " << expected << std::endl;
            errors++;
        }
    }

    if (errors == 0) {
        std::cout << filename << ": all " << lineNum << " lines passed." << std::endl;
        return true;
    } else {
        std::cout << filename << ": " << errors << " error(s) found." << std::endl;
        return false;
    }
}

int main() {
    bool allOk = true;

    // Check sin_results.txt
    allOk &= checkFile("sin_results.txt", "sin",
                       [](double x) { return std::sin(x); },
                       parseFuncLine);

    // Check sqrt_results.txt
    allOk &= checkFile("sqrt_results.txt", "sqrt",
                       [](double x) { return std::sqrt(x); },
                       parseFuncLine);

    // Check pow_results.txt
    allOk &= checkFile("pow_results.txt", "pow",
                       [](double x) { return x * x; },
                       parsePowLine);

    if (allOk) {
        std::cout << "\nAll tests passed successfully!" << std::endl;
        return 0;
    } else {
        std::cout << "\nSome tests failed." << std::endl;
        return 1;
    }
}
