cmake_minimum_required (VERSION 2.6)
project (Philosophen C)

set(CMAKE_C_FLAGS "-std=c99 -fopenmp")

add_executable(Philosophen philosophen.c)
target_link_libraries(Philosophen c)