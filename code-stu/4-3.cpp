#include <inttypes.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
/**规约
 * 将消息集中起来处理，与广播刚好相反
 * MPI_Reduce
 */
int32_t main(int argc, char *argv[]) {
  int32_t rank, size;
  int64_t data_a, data_b;
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  printf("rank == %" PRId32 " \n", rank);
  data_b = 0;
  data_a = rank + 10;
  MPI_Allreduce(&data_a, &data_b, 1, MPI_INT64_T, MPI_SUM, MPI_COMM_WORLD);
  /**
   * MPI_Allreduce =MPI_Reduce + MPI_Bcast
   * 先 规约后，处理好所有数据后
   * 再 广播到所有 进程
   */
  if (0 == rank) {
    printf("process %" PRId32 " total %" PRId64 " \n", rank, data_b);
  }
  MPI_Finalize();
  return 0;
}