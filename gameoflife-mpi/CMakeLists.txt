cmake_minimum_required(VERSION 3.9)
project(gameoflife_mpi C)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

add_executable(gameoflife_mpi main.c)
target_link_libraries(gameoflife_mpi ${MPI_LIBRARIES})

if(MPI_COMPILE_FLAGS)
    set_target_properties(gameoflife_mpi COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
    set_target_properties(gameoflife_mpi COMPILE_FLAGS "${MPI_LINK_FLAGS}")
endif()