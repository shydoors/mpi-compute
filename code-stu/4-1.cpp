#include <inttypes.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
int main(void) {
  int32_t rank, size;
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  printf("rank == %" PRId32 ", size == %" PRId32 "\n", rank, size);
  int32_t n = 0;
  if (0 == rank) {
    printf("pls enter n \n");
    scanf("%" PRId32 "", &n);
  }
  MPI_Bcast(&n, 1, MPI_INT32_T, 0, MPI_COMM_WORLD);
  /**MPI_Bcast 中，root变量，表示从 进程（rank == root ） 广播数据*/
  printf(">>>>>>>> rank %" PRId32 " , got %" PRId32 " \n", rank, n);
  MPI_Finalize();
  return 0;
}