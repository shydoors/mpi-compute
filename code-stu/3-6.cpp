#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
/**
 *
 * 不够高效
 *
 * rank 1 先接收 rank 2 的数据
 *
 * 等 rank 2 发完数据后，rank 1 再接收 rank 0 的数据
 */
int main(int argc, char *argv[]) {
  int32_t rank, size;
  MPI_Status stat;
  int32_t data_a, data_b;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  printf("rank == %d \n", rank);
  if (0 == rank) {
    sleep(5);
    data_a = 222;
    MPI_Send(&data_a, 1, MPI_INT32_T, 1, 1, MPI_COMM_WORLD);
    printf("Rank 0 sent data_a == %d \n", data_a);
    printf("Rank 0 is done\n");
  } else if (2 == rank) {
    data_a = 111;
    MPI_Send(&data_a, 1, MPI_INT32_T, 1, 1, MPI_COMM_WORLD);
    printf("Rank 2 sent data_a == %d \n", data_a);
    printf("Rank 2 is done\n");
  } else if (1 == rank) {
    MPI_Recv(&data_b, 1, MPI_INT32_T, 2, 1, MPI_COMM_WORLD, &stat);
    printf("Rank 1 received data_b == %d  frome rank==2\n", data_b);
    MPI_Recv(&data_b, 1, MPI_INT32_T, 0, 1, MPI_COMM_WORLD, &stat);
    printf("Rank 1 received data_b == %d \n", data_b);
    // MPI_Recv(&data_b, 1, MPI_INT32_T, MPI_ANY_SOURCE, MPI_ANY_TAG,
    // MPI_COMM_WORLD, &stat); printf("Rank 1 received data_b == %d \n",
    // data_b);
  }
  MPI_Finalize();
  return 0;
}
/**
 * 这是一个演示mpi程序
 * rank 0 和 rank 2 都向 rank 1 发送数据
 * rank1 先接收 rank 2 的数据，再接收 rank 0 的数据
 */