#include <inttypes.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
int main() {
  int32_t rank, size;
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  int32_t n = 1000;
  int32_t chunk = n / size;
  double sub_sum = 0.0, sum = 0.0;
  printf("rank == %" PRId32 " , chunk == %" PRId32 " \n", rank, chunk);
  for (int32_t i = rank * chunk; i < (rank + 1) * chunk; ++i) {
    double factor = (i % 2 == 0) ? 1.0 : -1.0;
    double a_i = (2 * i + 2) * (2 * i + 3) * (2 * i + 4);
    sub_sum += factor / a_i;
  }
  MPI_Reduce(&sub_sum, &sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  if (0 == rank) {
    if (n % size != 0) {
      for (int32_t i = chunk * size; i < n; ++i) {
        double factor = (i % 2 == 0) ? 1.0 : -1.0;
        double a_i = (2 * i + 2) * (2 * i + 3) * (2 * i + 4);
        sum += factor / a_i;
      }
    }
    printf("Pi == %f\n", 3 + 4 * sum);
  }
  MPI_Finalize();
  return 0;
}
/**
 * Pi= 3+ 4*(factor / a[i])
 *  factor = (i % 2 == 0) ? 1.0 : -1.0;
 * a[i] = (2 * i + 2) * (2 * i + 3) * (2 * i + 4);
 *  i >= 0 && i<n
 */