all: gameoflife_mpi

gameoflife_mpi: main.c
	mpicc -Wall -std=c99 -lc -D _BSD_SOURCE $< -o $@

.PHONY: run
run: gameoflife_mpi
	mpirun -n 4 ./$<
