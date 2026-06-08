#include <inttypes.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
/**计算 func在 [0,1]上的积分 */
double func(double dx) { return dx * dx; }
int main() {
  int32_t rank, size;
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  printf("rank == %" PRId32 " \n", rank);
  double sub_sum = 0.0, sum = 0.0, dx = 1.0 / size;
  sub_sum=func(dx*(rank+0.5))*dx;
  MPI_Reduce(&sub_sum, &sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  if (0 == rank) {
    printf("%lf\n", sum);
  }
  MPI_Finalize();
  return 0;
}