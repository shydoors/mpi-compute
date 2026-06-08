/**
 * 散射
 * 选定数据，发送给所有进程
 */
#include <inttypes.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define N 2
int main(void) {
  int32_t i, rank, size;
  int64_t *data_a, *data_b;
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  printf(" rank == %" PRId32 " \n", rank);
  data_a = (int64_t *)malloc(size * N * sizeof(int64_t));
  data_b = (int64_t *)malloc(N * sizeof(int64_t));
  if (0 == rank) {
    printf(">>>>>>>>>>>>>>>>\n");
    printf("data declspec \n");
    for (i = 0; i < size * N; ++i) {
      data_a[i] = i + 10;
      printf("rank == %" PRId32 " ,send data[%" PRId32 "] == %" PRId64 "\n",
             rank, i, data_a[i]);
    }
    printf("\n");
  }
  MPI_Scatter(data_a, N, MPI_INT64_T, data_b, N, MPI_INT64_T, 0,
              MPI_COMM_WORLD);
  for (i = 0; i < N; ++i) {
    printf(">>>>>>>> recv data[%" PRId32 "] == %" PRId64 "\n", i, data_b[i]);
  }
  free(data_a);
  free(data_b);
  MPI_Finalize();
  return 0;
}
/**
 * 虽然说是选择数据分发
 * 但是，实际上只能均匀选择
 * 本代码中 ，
 * data_a数组 申请了 N*size 个数据，
 * 但分发时，只能均匀切割为 size 个小数组
 * 然后从root进程广播给其他进程
 * 包括本身的进程
 */