#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int main(int argc, char *argv[]) {
    int comm_rank, comm_size;
    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(comm, &comm_size);
    MPI_Comm_rank(comm, &comm_rank);

    int height = 30, width = 30, argumentnr = 1;

    // Parse Height
    if(argumentnr < sizeof(argv)){
        height = atol(argv[argumentnr]);
        argumentnr++;
    }

    // Parse Width - if non existing set width equal to height
    if(argumentnr < sizeof(argv)){
        width = atol(argv[argumentnr]);
    } else {
        width = height;
    }
    argumentnr++;


    int total_length = height * comm_size;
    int partition = total_length / comm_size;
    int offset_start = partition * comm_rank;
    int offset_end = (offset_start + partition < total_length)? offset_start + partition -1 : total_length -1;

    printf("[INIT] Process %d of %d started with size of %dx%d - assigned partition %d-%d\n",
           comm_rank, comm_size, height, width, offset_start, offset_end);


    MPI_Finalize();
    return 0;
}