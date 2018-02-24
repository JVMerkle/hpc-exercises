#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

#include <omp.h>

#define calcIndex(width, x,y)  ((y)*(width) + (x))
#define SLICE_SIZE 30

long TimeSteps = 100;

void writeVTK2(char filename[2048], double *data, char prefix[1024], long w, long h) {
  int x,y;
  
  long offsetX=0;
  long offsetY=0;
  float deltax=1.0;
  float deltay=1.0;
  long  nxy = w * h * sizeof(float);  

  FILE* fp = fopen(filename, "w");

  fprintf(fp, "<?xml version=\"1.0\"?>\n");
  fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
  fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"0 0 0\" Spacing=\"%le %le %le\">\n", offsetX, offsetX + w, offsetY, offsetY + h, 0, 0, deltax, deltay, 0.0);
  fprintf(fp, "<CellData Scalars=\"%s\">\n", prefix);
  fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
  fprintf(fp, "</CellData>\n");
  fprintf(fp, "</ImageData>\n");
  fprintf(fp, "<AppendedData encoding=\"raw\">\n");
  fprintf(fp, "_");
  fwrite((unsigned char*)&nxy, sizeof(long), 1, fp);

  for (y = h-1; y >= 0; y--) {
    for (x = 0; x < w; x++) {
      float value = data[calcIndex(w, x,y)];
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

void evolve(double* currentfield, double* newfield, int w, int h) {
	for (int y = 0; y < SLICE_SIZE; y++) {
		for (int x = 0; x < SLICE_SIZE; x++) {

			int number = count_living_neighbours(currentfield, x, y, w, h);
			if (currentfield[calcIndex(w, x, y)] > 0) {
				number--;
			}

			newfield[calcIndex(w, x, y)] = (number == 3 || (number == 2 && currentfield[calcIndex(w, x, y)]))? 1 : 0;
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

	filling(currentfield, w, h);

	omp_set_num_threads(sqr_block_number * sqr_block_number);
	#pragma omp parallel
	{
        int thread_num = omp_get_thread_num();
        int offset_y = (thread_num / sqr_block_number) * SLICE_SIZE;
        int offset_x = (thread_num % sqr_block_number) * SLICE_SIZE;

		long t;
		for (t=0;t<TimeSteps;t++) {
			#pragma omp single
			{
				show(currentfield, w, h);
			}

            int index = calcIndex(w, offset_x, offset_y);

            evolve(&currentfield[index], newfield, w, h);
			#pragma omp barrier

            printf("%ld timestep\n", t);
            char filename[2048];
            snprintf(filename, sizeof(filename), "t%d-%05ld%s", thread_num, t, ".vti");


            writeVTK2(filename, currentfield, "gol", w, h);

			getchar();


			#pragma omp single
			{
				//SWAP
				double *temp = currentfield;
				currentfield = newfield;
				newfield = temp;
			};
		}
	}

	free(currentfield);
	free(newfield);

}
 
int main(int c, char **v) {
  int n = 0;
  if (c > 1) n = atoi(v[1]);
  if (n <= 0) n = 3;

  game(n, n * SLICE_SIZE, n * SLICE_SIZE);
}
