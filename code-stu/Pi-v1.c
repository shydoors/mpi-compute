#include <inttypes.h>
#include <math.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
double f(double x) { return 4.0 / (1.0 + x * x); }
int main(int argc, char *argv[]) {
  int32_t myid, numprocs;
  int64_t n, i; // 使用 long long 支持大 n
  double mypi, pi, h, sum, x;
  double start_time, end_time;
  const double PI_REF = 3.14159265358979323846;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  /**从命令行读取 n，默认 1000000*/
  if (argc > 1) {
    n = atoll(argv[1]);
  } else {
    n = 1000000LL;
  }
  if (0 == myid) {
    printf("Number of processes: %d\n", numprocs);
    printf("Number of intervals: %lld\n", n);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  start_time = MPI_Wtime();
  h = 1.0 / (double)n;
  sum = 0.0;
  /**每个进程计算自己负责的部分*/
  for (i = myid + 1; i <= n; i += numprocs) {
    x = h * ((double)i - 0.5);
    sum += f(x);
  }
  mypi = h * sum;
  /**规约求和到进程 0*/
  MPI_Reduce(&mypi, &pi, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  end_time = MPI_Wtime();

  if (0 == myid) {
    printf("Calculated pi  = %.15f\n", pi);
    printf("Reference pi   = %.15f\n", PI_REF);
    printf("Absolute error = %.15e\n", fabs(pi - PI_REF));
    printf("Wall-clock time = %.6f seconds\n", end_time - start_time);
  }
  MPI_Finalize();
  return 0;
}