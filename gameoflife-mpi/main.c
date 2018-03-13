#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "mpi.h"

int main(int argc, char *argv[]) {
    int comm_world_rank, comm_world_size;
    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(comm, &comm_world_size);
    MPI_Comm_rank(comm, &comm_world_rank);

    // ----- Create own gol communicator -----
    MPI_Comm comm_gol_comm;
    int dims = {comm_world_size};
    int periods = {true};
    int a = MPI_Cart_create(comm, 1, &dims, &periods, false, &comm_gol_comm);
    
    int comm_gol_rank, comm_gol_size;
    MPI_Comm_size(comm, &comm_gol_size);
    MPI_Comm_rank(comm, &comm_gol_rank);

    // ----- Find Neighbours -----
    //TODO: Implement

    // ----- Parse Inputs -----
    int height = 30, width, argumentnr = 1;

    // Parse Height
    if(argumentnr < argc){
        height = atoi(argv[argumentnr]);
        argumentnr++;
    }

    // Parse Width - if not existing: set width equal to height
    width = argumentnr < argc ? atoi(argv[argumentnr]) : height;

    // -----  -----
    int total_length = height * comm_gol_size;
    int partition = total_length / comm_gol_size;
    int offset_start = partition * comm_gol_rank;
    int offset_end = (offset_start + partition < total_length)? offset_start + partition -1 : total_length -1;

    printf("[INIT] Process %d of %d started with size of %dx%d - assigned partition %d-%d\n",
           comm_gol_rank, comm_gol_size, height, width, offset_start, offset_end);



    MPI_Finalize();
    return 0;
}