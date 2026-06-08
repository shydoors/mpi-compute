#include <inttypes.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
double f(double );
double f(double x) { return (4.0 / (1.0 + x * x)); }
int main(int argc, char *argv[]) {
  int myid, numprocs;
  int n, i;
  double mypi, pi;
  double h, sum, x;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  printf("Process %d of %d.\n", myid, numprocs);
  n = 100;
  h = 1.0 / (double)n;
  sum = 0.0;
  for (i = myid + 1; i <= n; i += numprocs) {
    x = h * ((double)i - 0.5);
    sum += f(x);
  }
  mypi = h * sum;
  MPI_Reduce(&mypi, &pi, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  if (myid == 0) {
    printf("\nprocess %d\n", myid);
    printf("The result is %.10f.\n", pi);
  }
  MPI_Finalize();
}