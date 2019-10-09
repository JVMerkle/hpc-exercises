cmake_minimum_required (VERSION 2.6)
project (Error2 C)

set(CMAKE_C_FLAGS "-std=c99 -fopenmp")

add_executable(Error2 error2.c)
target_link_libraries(Error2 c)