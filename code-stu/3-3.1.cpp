#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
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
    MPI_Send(&data_send, 1, MPI_INT32_T, 1, 1, MPI_COMM_WORLD);
    printf("all data:\n");
    printf("data_send : %d\n", data_send);
    printf("data_recv : %d\n", data_recv);
  } else if (1 == rank) {
    printf("all data:\n");
    printf("data_send : %d\n", data_send);
    // printf("data_recv : %d\n", data_recv);
    MPI_Recv(&data_recv, 1, MPI_INT32_T, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD,
             &stat);
    printf("data_recv : %d\n", data_recv);
  }
  MPI_Finalize();
  return 0;
}