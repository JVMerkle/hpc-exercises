cmake_minimum_required (VERSION 2.6)
project (Error1 C)

set(CMAKE_C_FLAGS "-std=c99 -fopenmp")

add_executable(Error1 main.c)
target_link_libraries(Error1 c)