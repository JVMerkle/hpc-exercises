#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "mpi.h"

#define calcIndex(width, x, y)  ((y)*(width) + (x))

void writeVTK(char *filename, const char *field, int block_width, int block_height, int total_width, int total_height,
           int offset_x, int offset_y);

void *init_field(int rank, char *current_field, int length) {
    srand(rank*time(NULL));
    for (int i = 0; i < length; i++) {
        current_field[i] = (char) ((rand() < RAND_MAX / 10) ? 1 : 0);
    }
}

int main(int argc, char *argv[]) {
    int comm_world_rank, comm_world_size;
    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(comm, &comm_world_size);
    MPI_Comm_rank(comm, &comm_world_rank);

    // ----- Create own gol communicator -----
    MPI_Comm comm_gol;
    int dims = {comm_world_size};
    int periods = {true};
    MPI_Cart_create(comm, 1, &dims, &periods, false, &comm_gol);
    
    int comm_gol_rank, comm_gol_size;
    MPI_Comm_size(comm, &comm_gol_size);
    MPI_Comm_rank(comm, &comm_gol_rank);

    // ----- Find Neighbours -----
    int next_neighbour;
    int previous_neighbour;
    MPI_Cart_shift(comm_gol, 0, 1, &previous_neighbour, &next_neighbour);


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
    int total_length = height * width;
    int proc_height = height / comm_gol_size;
    int proc_area = proc_height * width;
    int offset = proc_area * comm_gol_rank;

    printf("[INIT] Process %d of %d started with size of %dx%d - assigned partition %d next rank: %d previous rank: %d\n",
           comm_gol_rank, comm_gol_size, height, width, offset,
           next_neighbour, previous_neighbour);

    // Initialise fields
    char *currentField = calloc((size_t) proc_area + 2 * width, sizeof(char));
    char *nextField = calloc((size_t) proc_area + 2 * width, sizeof(char));

    init_field(comm_gol_rank, currentField + width, proc_area);

    char thread_filename[2048];
    snprintf(thread_filename, sizeof(thread_filename), "gol-%05d-r%d%s", 0, comm_gol_rank, ".vti");
    writeVTK(thread_filename, currentField + width, width, proc_height, width, height, 0, comm_gol_rank * proc_height);

    MPI_Finalize();
    return 0;
}


void writeVTK(char *filename, const char *field, int block_width, int block_height, int total_width, int total_height,
           int offset_x, int offset_y) {
    int x, y;
    float deltax = 1.0;
    float deltay = 1.0;
    long nxy = total_width * total_height * sizeof(float);

    FILE *fp = fopen(filename, "w");

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
    fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"0 0 0\" Spacing=\"%le %le %le\">\n", offset_x,
            offset_x + block_width, total_height - offset_y - block_height, total_height - offset_y, 0, 0, deltax,
            deltay, 0.0);
    fprintf(fp, "<CellData Scalars=\"%s\">\n", "gol");
    fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", "gol");
    fprintf(fp, "</CellData>\n");
    fprintf(fp, "</ImageData>\n");
    fprintf(fp, "<AppendedData encoding=\"raw\">\n");
    fprintf(fp, "_");
    fwrite((unsigned char *) &nxy, sizeof(long), 1, fp);

    for (y = block_height - 1; y >= 0; y--) {
        for (x = 0; x < block_width; x++) {
            float value = field[calcIndex(total_width, x, y)];
            fwrite((unsigned char *) &value, sizeof(float), 1, fp);
        }
    }

    fprintf(fp, "\n</AppendedData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}
