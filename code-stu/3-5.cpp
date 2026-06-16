#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
int main(void) {
  int32_t rank, size, data_tmp;
  MPI_Status stat;
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  printf("process is rank "PRId32", size is "PRId32"\n", rank, size);
  if (0 == rank) {
    data_tmp = 666;
    printf("Send data  "PRId32"\n", data_tmp);
    MPI_Send(&data_tmp, 1, MPI_INT, 3, 1, MPI_COMM_WORLD);
    data_tmp = 777;
    printf("Send data  "PRId32"\n", data_tmp);
    MPI_Send(&data_tmp, 1, MPI_INT, 3, 2, MPI_COMM_WORLD);

    /**
     * 这里说明一下
     * 第二个参数是count
     * 设置为3时，会从 data_tmp向后寻找3三个数据进行发送
     * 此处是寻找3个int类型的变量，即 datat_tmp[0],datat_tmp[1],datat_tmp[2]
     */

    // 如果想将同一个数据发送3次，建议用for循环
    /*
    for (int i = 0; i < 3; i++) {
      MPI_Send(&data_tmp, 1, MPI_INT, 3, 1, MPI_COMM_WORLD);
    }

    */
  } else if (3 == rank) {
    // printf("hello world from rank %d\n", rank);
    int32_t data = 0;
    MPI_Recv(&data, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &stat);
    printf("Rank "PRId32" got msg data "PRId32"\n", rank, data);
  }
  MPI_Finalize();
  return 0;
}
