#ifdef _WIN32
#include <mem.h>
#endif

#ifdef linux
#include <memory.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#include <stdbool.h>

#define calcIndex(width, x, y)  ((y)*(width) + (x))

#define TIME_STEPS 100

void game(char *filename, int width, int height, int blocks_x, int blocks_y);

char *init_field(char *current_field, char *filename, int width, int height);

void
evolve(const char *current_field, char *new_field, int total_width, int total_height, int offset_x, int offset_y);

int count_living_neighbours(const char *field, int x, int y, int width, int height);

void print_field(const char *field, int width, int height);

void write(char *filename, const char *field, int block_width, int block_height, int total_width, int total_height,
           int offset_x, int offset_y);

bool print = true;

int main(int argc, char *argv[]) {

    char *filename = "";
    int width = 10, height = 10, blocks_x = 3, blocks_y = 3;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-i") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "ERROR: Missing file parameter");
                return 1;
            }
            filename = argv[i];
        } else if (strcmp(argv[i], "-s") == 0) {
            i++;
            width = atol(argv[i]);
            i++;
            if (i >= argc) {
                height = width;
                break;
            }
            height = atol(argv[i]);
        } else if (strcmp(argv[i], "-b") == 0) {
            i++;
            blocks_x = atol(argv[i]);
            i++;
            if (i >= argc) {
                blocks_y = blocks_y;
                break;
            }
            blocks_y = atol(argv[i]);
        } else if (strcmp(argv[i], "--no-print") == 0 || strcmp(argv[i], "-np") == 0) {
            print = false;
        }
    }

    game(filename, width, height, blocks_x, blocks_y);

    return 0;
}

void game(char *filename, int width, int height, int blocks_x, int blocks_y) {
    int total_width = width * blocks_x;
    int total_height = height * blocks_y;

    char *current_field = calloc((size_t) (total_height * total_width), sizeof(char));
    char *new_field = calloc((size_t) (total_height * total_width), sizeof(char));
    init_field(current_field, filename, total_width, total_height);

    double cpu_time_used_total = 0;
    clock_t start = clock(), end;

#pragma omp parallel num_threads(blocks_x * blocks_y)
    {
        int thread_num = omp_get_thread_num();
        int offset_y = (thread_num / blocks_y) * height;
        int offset_x = (thread_num % blocks_x) * width;

        for (int t = 0; t < TIME_STEPS; ++t) {

            if(print) {
#pragma omp single
                print_field(current_field, total_width, total_height);
            }
            //printf("Thread %d at subfield position %d, offset_x: %d, offset_y: %d\n", thread_num, calcIndex(total_width, offset_x, offset_y), offset_x, offset_y);

            evolve(current_field, new_field, total_width, total_height, offset_x, offset_y);

            char thread_filename[2048];
            snprintf(thread_filename, sizeof(thread_filename), "t%d-%05d%s", thread_num, t, ".vti");
            write(thread_filename, current_field, width, height, total_width, total_height, offset_x, offset_y);

#pragma omp barrier
#pragma omp single
            {
                char *tmp = current_field;
                current_field = new_field;
                new_field = tmp;

                end = clock();
                double cpu_time_used = ((end - start) * 1000.0) / CLOCKS_PER_SEC;
                cpu_time_used_total += cpu_time_used;

                printf("Time step: %d CPU time: %.3f ms\n", t, cpu_time_used);
                if(print) {
                    getchar();
                }
                start = clock();
            }
        }
    }

    printf("\n----- -----\n");
    printf("Average CPU time: %.3f ms\n", cpu_time_used_total / TIME_STEPS);
}

void write(char *filename, const char *field, int block_width, int block_height, int total_width, int total_height,
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
    fprintf(fp, "<CellData Scalars=\"%s\">\n", "th");
    fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", "th");
    fprintf(fp, "</CellData>\n");
    fprintf(fp, "</ImageData>\n");
    fprintf(fp, "<AppendedData encoding=\"raw\">\n");
    fprintf(fp, "_");
    fwrite((unsigned char *) &nxy, sizeof(long), 1, fp);

    for (y = offset_y + block_height - 1; y >= offset_y; y--) {
        for (x = offset_x; x < offset_x + block_width; x++) {
            float value = field[calcIndex(total_width, x, y)];
            fwrite((unsigned char *) &value, sizeof(float), 1, fp);
        }
    }

    fprintf(fp, "\n</AppendedData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}

void
evolve(const char *current_field, char *new_field, int total_width, int total_height, int offset_x, int offset_y) {
    for (int y = offset_y; y < total_height; ++y) {
        for (int x = offset_x; x < total_width; ++x) {
            int index = calcIndex(total_width, x, y);
            int neighbours = count_living_neighbours(current_field, x, y, total_width, total_height);
            new_field[index] = (neighbours == 3 || (neighbours == 2 && current_field[index]));
        }
    }
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

char *init_field(char *current_field, char *filename, int width, int height) {
    if (filename != "") {
        FILE *fp;
        fp = fopen(filename, "r");

        if (fp != NULL) {
            for (int y = 0; y < height; ++y) {
                char *line = NULL;
                size_t len = 0;
                ssize_t read = getline(&line, &len, fp);

                for (int x = 0; x < width; ++x) {
                    current_field[calcIndex(width, x, y)] = (char) (x < read && (line[x] == 'X' || line[x] == 'x') ? 1
                                                                                                                   : 0); //TODO: Is cast necessary
                }
            }
        } else {
            fprintf(stderr, "WARNING: Could not open file");
        }
    } else {
        for (int i = 0; i < width * height; i++) {
            current_field[i] = (char) ((rand() < RAND_MAX / 10) ? 1 : 0);
        }
    }

    return calloc(10, sizeof(char));
}

void print_field(const char *field, int width, int height) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            printf("%c", field[calcIndex(width, x, y)] ? 'X' : ' ');
        }
        printf("|\n");
    }
}
