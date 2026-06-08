#include <inttypes.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define N 10
/**
 * Allgather
 *
 * 整合为一个完整的数组，然后分发给所有子进程，包括root进程
 *
 * 所以函数没有目标进程
 */
int main(int argc, char *argv[]) {
  int32_t rank, size, i;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  int64_t *data_a, *data_b;
  data_a = (int64_t *)malloc(size * N * sizeof(int64_t));
  data_b = (int64_t *)malloc(N * sizeof(int64_t));
  /**赋值*/
  for (i = 0; i < N; ++i) {
    data_b[i] = i + rank * 10;
  }
  /**传递*/
  MPI_Allgather(data_b, N, MPI_INT64_T, data_a, N, MPI_INT64_T, MPI_COMM_WORLD);
  // 我不太确定，能不能用这个auto，因为我是写在cpp文件里的，但是
  // 导入的是c语言库，外加一个mpi.h
  //   for(auto i:data_a){
  //     std::cout<<i<<std::endl;
  //   }
  /**输出*/
  if (0 == rank) {
    printf("data get \n");
    for (i = 0; i < N * size; ++i) {
      printf("data_a[%" PRId32 "]= %" PRId64 "\n", i, data_a[i]);
    }
  }
  free(data_a);
  free(data_b);
  MPI_Finalize();
  return 0;
}