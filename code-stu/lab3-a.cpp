/**
 * MPI通信的基本要求：
 *
 * 1. 当两个进程属于同一个通信域，才能彼此通信。
 * 2. 发送端的dest参数指定了要接收消息进程的进程号，而接收段的source
 * 参数指定了发送消息进程的进程号，两者要配对，否则就接收不到消息。
 *  有意思的是，source参数还可以指定为MPI_ANY_SOURCE，表示接受来自任何进程的消息，这样就不需要关心发送消息的进程号了。
 * 3. 两个进程间的标签tag 要相同。
 * tag也可以指定为 MPI_ANY_TAG，表示接受任何标签的消息。
 *
 *
 * 详情可以直接打印 MPI_Status 结构体的成员来查看。
 * MPI_Status 结构体的成员包括：
 * - MPI_SOURCE: 发送消息的进程rank。
 * - MPI_TAG: 消息标签。
 * - MPI_ERROR: 错误代码，表示接收消息时发生的错误。
 */
#include <inttypes.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
int main(void) {
  int32_t size, rank, data_send = 1, data_recv;
  MPI_Status stat;
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  printf(" rank == %d\n", rank);
  if (0 == rank) {
    data_send = 777;
    MPI_Send(&data_send, 1, MPI_INT32_T, 1, 1, MPI_COMM_WORLD);
    printf("Rank %d : send data which value is %" PRId32 " \n", rank,
           data_send);
  } else if (1 == rank) {
    MPI_Recv(&data_recv, 1, MPI_INT32_T, 0, 1, MPI_COMM_WORLD, &stat);
    printf("Rank %d : recv data which value is %" PRId32 " \n", rank,
           data_recv);
  }
  MPI_Finalize();
  return 0;
}