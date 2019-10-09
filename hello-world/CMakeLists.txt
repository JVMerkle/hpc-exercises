cmake_minimum_required (VERSION 2.6)
project (HelloWorld C)

set(CMAKE_C_FLAGS "-std=c99 -fopenmp")

add_executable(HelloWorld hello-world.c)
target_link_libraries(HelloWorld c)