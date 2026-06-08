#include <mpi.h>
#include <stdio.h>

int main(int argc, char **argv) {
  int rank = 0, size = 0, len = 0;
  char version[MPI_MAX_LIBRARY_VERSION_STRING];
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Get_library_version(version, &len);
  //   printf("I from process %d ,and size = %d \n",rank, size);
  printf("%s\n", version);
  printf("the version is %s , the Len %d\n", version, len);
  MPI_Finalize();
  return 0;
}