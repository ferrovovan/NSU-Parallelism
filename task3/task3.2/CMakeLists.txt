cmake_minimum_required(VERSION 3.0)
project(task3.2)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

add_executable(task3.2 task3.2.cpp)

target_link_libraries(task3.2 PRIVATE Threads::Threads)
