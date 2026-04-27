cmake_minimum_required(VERSION 3.10)
project(task2_3 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Global compilation flags (you can override CMAKE_CXX_FLAGS externally)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3")

# OpenMP
find_package(OpenMP REQUIRED)

# Default values for preprocessor definitions (override with -D EPS=... etc.)
set(EPS "1e-5" CACHE STRING "Convergence tolerance")
set(TAU "1e-4" CACHE STRING "Method parameter")
set(N   "13960" CACHE STRING "Problem dimension")

# Helper to create one executable variant
function(add_method_variant TARGET_NAME METHOD_SOURCE)
  add_executable(${TARGET_NAME}
        task2.3_main.cpp    # common main
        ${METHOD_SOURCE}    # specific implementation
        config.hpp          # optional: helps dependency tracking
        method.hpp
    )
  target_compile_definitions(${TARGET_NAME} PRIVATE
        EPS=${EPS}
        TAU=${TAU}
        N=${N}
    )
  target_link_libraries(${TARGET_NAME} PRIVATE
        OpenMP::OpenMP_CXX   # handles compiler/linker flags for OpenMP
        m                    # math library (equivalent to -lm)
    )
endfunction()

# Build three executables
add_method_variant(task2.3-for     method_for.cpp)
add_method_variant(task2.3-manual  method_manual.cpp)
add_method_variant(task2.3-schedule method_schedule.cpp)

# Convenience 'test' target that runs all three
add_custom_target(test
    COMMAND task2.3-for
    COMMAND task2.3-manual
    COMMAND task2.3-schedule
    DEPENDS task2.3-for task2.3-manual task2.3-schedule
    COMMENT "Running all three method variants"
)

# Also register them as CTest tests
enable_testing()
add_test(NAME test_for      COMMAND task2.3-for)
add_test(NAME test_manual   COMMAND task2.3-manual)
add_test(NAME test_schedule COMMAND task2.3-schedule)
