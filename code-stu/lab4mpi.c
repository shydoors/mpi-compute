#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
int main(int argc, char *argv[]) {
  int32_t rank, size;
  MPI_Status status;
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  if (0 == rank) {
    MPI_Send("Hello, World!", 14, MPI_CHAR, 10, 0, MPI_COMM_WORLD);
  } else if (10 == rank) {
    char buffer[15];
    MPI_Recv(buffer, 14, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
    printf("Received: %s\n", buffer);
    printf("From rank: %d\n", status.MPI_SOURCE);
    printf("From rank: %d\n", status.MPI_SOURCE);
    printf("From tag: %d\n", status.MPI_TAG);
  }
  MPI_Finalize();
  return 0;
}