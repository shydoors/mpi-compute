#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
/**
 * 典型的MPI死锁，慎用
 */
int main(void) {
  int32_t size, rank, data_send = 1, data_recv;
  MPI_Status stat;
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  printf(" rank == %d\n", rank);
  if (0 == rank) {
    data_send = 777;
    MPI_Send(&data_send, 1, MPI_INT, 1, 1, MPI_COMM_WORLD);
    printf("rank "PRId32" sent data : "PRId32" \n", rank, data_send);
    data_send = 888;
    MPI_Send(&data_send, 1, MPI_INT, 1, 2, MPI_COMM_WORLD);
    printf("rank "PRId32" sent data : "PRId32" \n", rank, data_send);
  } else if (1 == rank) {
    MPI_Recv(&data_recv, 1, MPI_INT, 0, 3, MPI_COMM_WORLD, &stat);
    printf("rank "PRId32" got data : "PRId32" \n", rank, data_recv);
  }
  MPI_Finalize();
  return 0;
}
/**
 * 分析：
 * 1. 进程0发送了两条消息，第一条消息的标签为1，第二条消息的标签为2。
 * 2. 进程1试图接收一条消息，指定的标签为3。
 * 3. 由于进程1等待的是标签为3的消息，而进程0发送的消息标签分别为1和2，因此进程1将永远等待，导致死锁。
 * 4. 解决方法：确保发送和接收的标签匹配，或者使用MPI_ANY_TAG来接受任何标签的消息。
 */