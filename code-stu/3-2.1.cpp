/**
 * author: shydoors
 * date: 2026-04-16
 */
#include <inttypes.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
int main(void) {
  int32_t rank, size, tag = 1;
  int64_t data_a, data_b;
  MPI_Status stat;
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  if (0 == rank) {
    data_a = 666;
    MPI_Send(&data_a, 1, MPI_INT, 1, tag, MPI_COMM_WORLD);
  } else if (1 == rank) {
    MPI_Recv(&data_b, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &stat);
    printf("rank %d got %" PRId64 "\n", rank, data_b);
  }
  MPI_Finalize();
  return 0;
}
/**
 * Send函数的参数说明：
 * - buf: 指向要发送数据的缓冲区的指针。
 * - count: 要发送的数据项的数量。
 * - datatype: 数据项的类型（如MPI_INT、MPI_FLOAT等）。
 * - dest: 目标进程的rank。
 * - tag: 消息标签，用于区分不同类型的消息。
 * - comm: 通信器，指定通信的范围（如MPI_COMM_WORLD）。
 *
 *
 * Recv函数的参数说明：
 * - buf: 指向接收数据的缓冲区的指针。
 * - count: 要接收的数据项的最大数量。
 * - datatype: 数据项的类型（如MPI_INT、MPI_FLOAT等）。
 * - source: 发送消息的进程rank，或者MPI_ANY_SOURCE表示接受来自任何进程的消息。
 * - tag: 消息标签，或者MPI_ANY_TAG表示接受任何标签的消息。
 * - comm: 通信器，指定通信的范围（如MPI_COMM_WORLD）。
 * - status:
 * 指向MPI_Status结构的指针，用于存储接收消息的状态信息，如发送者的rank、消息标签等。
 */