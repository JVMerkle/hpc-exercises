cmake_minimum_required (VERSION 2.6)
project (Pi C)

set(CMAKE_C_FLAGS "-std=c99 -fopenmp")

add_executable(Pi main.c)
target_link_libraries(Pi c)