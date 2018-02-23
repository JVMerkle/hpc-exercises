#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

#define calcIndex(width, x,y)  ((y)*(width) + (x))
#define SLICE_SIZE 30

long TimeSteps = 100;

void writeVTK2(long timestep, double *data, char prefix[1024], long w, long h) {
  char filename[2048];  
  int x,y; 
  
  long offsetX=0;
  long offsetY=0;
  float deltax=1.0;
  float deltay=1.0;
  long  nxy = w * h * sizeof(float);  

  snprintf(filename, sizeof(filename), "%s-%05ld%s", prefix, timestep, ".vti");
  FILE* fp = fopen(filename, "w");

  fprintf(fp, "<?xml version=\"1.0\"?>\n");
  fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
  fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"0 0 0\" Spacing=\"%le %le %le\">\n", offsetX, offsetX + w-1, offsetY, offsetY + h-1, 0, 0, deltax, deltax, 0.0);
  fprintf(fp, "<CellData Scalars=\"%s\">\n", prefix);
  fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
  fprintf(fp, "</CellData>\n");
  fprintf(fp, "</ImageData>\n");
  fprintf(fp, "<AppendedData encoding=\"raw\">\n");
  fprintf(fp, "_");
  fwrite((unsigned char*)&nxy, sizeof(long), 1, fp);

  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      float value = data[calcIndex(h, x,y)];
      fwrite((unsigned char*)&value, sizeof(float), 1, fp);
    }
  }
  
  fprintf(fp, "\n</AppendedData>\n");
  fprintf(fp, "</VTKFile>\n");
  fclose(fp);
}


void show(double* currentfield, int w, int h) {
  printf("\033[H");
  int x,y;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) printf(currentfield[calcIndex(w, x,y)] ? "\033[07m  \033[m" : "  ");
    //printf("\033[E");
    printf("\n");
  }
  fflush(stdout);
}
 
int count_living_neighbours(double *currentfield, int x, int y, int w, int h) {
    int number = 0;
    for (int iy = y - 1; iy <= y + 1; iy++) {
        for (int ix = x - 1; ix <= x + 1; ix++) {
            if (currentfield[calcIndex(w, (ix + w) % w, (iy + h) % h)] > 0) {
                number++;
            }
        }
    }
    return number;
} 

void evolve(double* currentfield, double* newfield, int sqr_block_number, int w, int h) {
  omp_set_num_threads(sqr_block_number * sqr_block_number);
  #pragma omp parallel
  {
    int thread_num = omp_get_thread_num();
    int offset_y = (thread_num / sqr_block_number) * SLICE_SIZE;
    int offset_x = (thread_num % sqr_block_number) * SLICE_SIZE;

    for (int y = offset_y; y < offset_y + SLICE_SIZE; y++) {
      for (int x = offset_x; x < offset_x + SLICE_SIZE; x++) {
        
        int number = count_living_neighbours(currentfield, x, y, w, h);
        if (currentfield[calcIndex(w, x, y)] > 0) {
          number--;
        }
        
        newfield[calcIndex(w, x, y)] = (number == 3 || (number == 2 && currentfield[calcIndex(w, x, y)]))? 1 : 0;
      }
    }
  }
}
 
void filling(double* currentfield, int w, int h) {
  int i;
  for (i = 0; i < h*w; i++) {
    currentfield[i] = (rand() < RAND_MAX / 10) ? 1 : 0; ///< init domain randomly
  }
}
 
void game(int sqr_block_number, int w, int h) {
  double *currentfield = calloc(w*h, sizeof(double));
  double *newfield     = calloc(w*h, sizeof(double));
  
  //printf("size unsigned %d, size long %d\n",sizeof(float), sizeof(long));
  
  filling(currentfield, w, h);
  long t;
  for (t=0;t<TimeSteps;t++) {
    show(currentfield, w, h);
    evolve(currentfield, newfield, sqr_block_number, w, h);
    
    printf("%ld timestep\n",t);
    writeVTK2(t,currentfield,"gol", w, h);
    
    usleep(200000);

    //SWAP
    double *temp = currentfield;
    currentfield = newfield;
    newfield = temp;
  }
  
  free(currentfield);
  free(newfield);
  
}

void measure_time(int sqr_block_number, int w, int h, int times) {
  double *currentfield = calloc(w*h, sizeof(double));
  double *newfield     = calloc(w*h, sizeof(double));

  double cpu_time_used_total = 0;
  for (int i = 0; i < times; i++)
  {
    filling(currentfield, w, h);

    printf("Round %d of %d: ", i+1, times);
    clock_t start = clock(), end;

    for (int j = 0; j < TimeSteps; j++) {
      evolve(currentfield, newfield, sqr_block_number, w, h);

      //SWAP
      double *temp = currentfield;
      currentfield = newfield;
      newfield = temp;
    }
    end = clock();

    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    cpu_time_used_total += cpu_time_used;
    printf("%.3f ms cpu-time\n", cpu_time_used * 1000);
  }
  
  free(currentfield);
  free(newfield);

  printf("\n----- -----\n");
  printf("Average: %f\n", cpu_time_used_total / times);
}
 
int main(int c, char **v) {
  int n = 0;
  if (c > 1) n = atoi(v[1]);
  if (n <= 0) n = 3;

  game(n, n * SLICE_SIZE, n * SLICE_SIZE);
  //measure_time(w, h, 20);
}
