# HPC Exercises
High-Performance-Computing exercises.

# Requirements
- GCC
- CMake

For gameoflife-mpi:
```bash
apt install openmpi-bin
```

# Building
```bash
mkdir cmake-build-debug
cd cmake-build-debug
cmake ..
make -j4
```