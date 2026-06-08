#include <inttypes.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define N 10
#define target_id 0
int main() {
  int32_t rank, size, i;
  int64_t *data_a, *data_b;
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  data_a = (int64_t *)malloc(size * N * sizeof(int64_t));
  data_b = (int64_t *)malloc(N * sizeof(int64_t));
  printf("rank == %" PRId32 " \n", rank);
  for (i = 0; i < N; ++i) {
    data_b[i] = i + 10 * rank;
  }
  MPI_Gather(data_b, N, MPI_INT64_T, data_a, N, MPI_INT64_T, target_id,
             MPI_COMM_WORLD);
  if (target_id == rank) {
    printf(">>>>>>>> recv data: \n");
    for (i = 0; i < size * N; ++i) {
      printf("data_a[%" PRId32 "] == %" PRId64 " \n", i, data_a[i]);
    }
  }
  /**
   * 注意顺序，先赋值，再输出，不然就全是0了
   *
   * linux下，接收的数据 按照rank升序排列的，windows目前没试过
   * 
   * 假设 recv_data[i] = send_data[j]
   * 其中 i=rank*size+j
   */
  free(data_a);
  free(data_b);
  MPI_Finalize();
  return 0;
}
/**
 * MPI相关函数都是有规律的
 * MPI函数的第一个参数基本都是
 * 发送方的，
 * 然后是数据发送个数，数据类型，相应的tag
 *
 * 这时候一般接接受方的参数
 * 和发送方一致，先是保存数据的变量，然后是数据多少，数据类型，相应的tag
 *
 * 之后就是发送的进程，接收的进程
 *
 * 最后的参数就是通信域，一般是MPI_COMM_WORLD
 */