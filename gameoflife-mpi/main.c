#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <memory.h>
#include "mpi.h"

#define calcIndex(width, x, y)  ((y)*(width) + (x))

bool evolve(const char *current_field, char *new_field, int block_width, int block_height, int width,
            int height, int offset_x, int offset_y);

void writeVTK(char *filename, const char *field, int block_width, int block_height, int total_width, int total_height,
           int offset_x, int offset_y);

int count_living_neighbours(const char *field, int x, int y, int width, int height) ;

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
    bool run = true;
    int i = 0;
    for (; run && i < 100; ++i) {

        // ----- Exchange ghost layer -----
        // Send to next neighbour
        MPI_Request send_next_request;
        char *send_next_buffer = currentField + proc_height * width;
        MPI_Isend(send_next_buffer, width, MPI_CHAR, next_neighbour, 1000, comm_gol, &send_next_request);

        // Send to previous neighbour
        MPI_Request send_previous_request;
        char *send_previous_buffer = currentField + width;
        MPI_Isend(send_previous_buffer, width, MPI_CHAR, previous_neighbour, 2000, comm_gol, &send_previous_request);
        //printf("[DEBUG P:%d] Invoked sending to: %d\n", comm_gol_rank, comm_gol_rank - 1 < 0 ? comm_gol_size - 1 : comm_gol_rank - 1);

        // Receive from previous neighbour
        MPI_Status receive_previous_status;
        char *receive_previous_buffer = calloc((size_t) width, sizeof(char));
        MPI_Recv(receive_previous_buffer, width, MPI_CHAR, previous_neighbour, 1000, comm_gol, &receive_previous_status);

        memcpy(currentField, receive_previous_buffer, (size_t) width);

        // Receive from next neighbour
        MPI_Status receive_next_status;
        char *receive_next_buffer = calloc((size_t) width, sizeof(char));
        MPI_Recv(receive_next_buffer, width, MPI_CHAR, next_neighbour, 2000, comm_gol, &receive_next_status);
        //printf("[DEBUG P:%d] Message received from %d\n", comm_gol_rank, comm_gol_rank + 1 >= comm_gol_size? 0 : comm_gol_rank + 1);

        memcpy(currentField + ((proc_height + 1) * width), receive_next_buffer, (size_t) width);

        // ----- Write VTK files -----
        char thread_filename[2048];
        snprintf(thread_filename, sizeof(thread_filename), "gol%d-%05d%s", comm_gol_rank, i, ".vti");
        writeVTK(thread_filename, currentField + width, width, proc_height, width, height, 0, comm_gol_rank * proc_height);

        // ----- evolve -----
        bool change = evolve(currentField, nextField, width, proc_height, width, proc_height + 2, 0, 1);
        char *tmp = currentField;
        currentField = nextField;
        nextField = tmp;

        // ----- exchange change -----
        bool *send_change_buffer = calloc((size_t) 1, sizeof(bool));
        send_change_buffer[0] = change;
        bool *receive_change_buffer = calloc((size_t) comm_gol_size, sizeof(bool));
        MPI_Allgather(send_change_buffer, 1, MPI_C_BOOL, receive_change_buffer, 1, MPI_C_BOOL, comm_gol);
        run = false;
        for (int j = 0; j < comm_gol_size; ++j) {
            if(receive_change_buffer[j]) {
                run = true;
                break;
            }
        }
    }

    printf("[DEBUG P:%d] Finished after %d steps\n", comm_gol_rank, i);

    MPI_Finalize();
    return 0;
}

bool evolve(const char *current_field, char *new_field, int block_width, int block_height, int total_width,
            int total_height, int offset_x, int offset_y) {
    bool change = false;
    for (int y = offset_y; y < offset_y + block_height; ++y) {
        for (int x = offset_x; x < offset_x + block_width; ++x) {
            int index = calcIndex(total_width, x, y);
            int neighbours = count_living_neighbours(current_field, x, y, total_width, total_height);
            new_field[index] = (neighbours == 3 || (neighbours == 2 && current_field[index]));
            if(!change && new_field[index] != current_field[index]) change = true;
        }
    }
    return change;
}

int count_living_neighbours(const char *field, int x, int y, int width, int height) {
    int number = 0;
    for (int iy = y - 1; iy <= y + 1; iy++) {
        for (int ix = x - 1; ix <= x + 1; ix++) {
            if (field[calcIndex(width, (ix + width) % width, (iy + height) % height)] > 0) {
                number++;
            }
        }
    }
    return number - field[calcIndex(width, x, y)];
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
