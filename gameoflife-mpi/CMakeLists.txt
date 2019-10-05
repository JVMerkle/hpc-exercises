cmake_minimum_required (VERSION 2.6)
project(GameOfLifeMpi C)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

add_executable(GameOfLifeMpi main.c)
target_link_libraries(GameOfLifeMpi ${MPI_LIBRARIES})

if(MPI_COMPILE_FLAGS)
    set_target_properties(GameOfLifeMpi PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS} ${MPI_LINK_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
    set_target_properties(GameOfLifeMpi PROPERTIES COMPILE_FLAGS "${MPI_LINK_FLAGS}")
endif()