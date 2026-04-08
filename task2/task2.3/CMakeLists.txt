cmake_minimum_required(VERSION 3.0)
project(task2.3)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

add_executable(task2.3-for      task2.3_for.cpp)
add_executable(task2.3-general  task2.3_general.cpp)
add_executable(task2.3-schedule task2.3_schedule.cpp)

target_link_libraries(task PRIVATE Threads::Threads)
