cmake_minimum_required (VERSION 2.6)
project (GameOfLife C)

set(CMAKE_C_FLAGS "-std=c99 -fopenmp")

add_executable(GameOfLife main.c)
target_link_libraries(GameOfLife c)