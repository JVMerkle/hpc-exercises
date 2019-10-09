cmake_minimum_required (VERSION 2.6)
project (ParalleStack C)

set(CMAKE_C_FLAGS "-std=c99 -fopenmp")

add_executable(ParalleStack main.c)
target_link_libraries(ParalleStack c)