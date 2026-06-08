#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
/**
 * MPI的简单演示程序
 * 用以演示 MRI_ANY_SOURCE 和 MPI_ANY_TAG 的使用
 */
int main(void) {
  int32_t rank, size;
  MPI_Status stat;
  int32_t data_a, data_b;
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  printf("rank == %d \n", rank);
  if (0 == rank) {
    data_a = 111;
    MPI_Send(&data_a, 1, MPI_INT32_T, 9, 0, MPI_COMM_WORLD);
    printf(">>>>>>>>>>>>> rank 0 sent data_a == %d  to rank 9\n", data_a);
    printf("\n");
  } else if (1 == rank) {
    data_a = 222;
    MPI_Send(&data_a, 1, MPI_INT32_T, 9, 1, MPI_COMM_WORLD);
    printf(">>>>>>>>>>>>> rank 1 sent data_a == %d to rank 9\n", data_a);
    printf("\n");
  } else if (9 == rank) {
    int flag = 0;
    printf("================ size : %d ==========\n", size);
    while (flag) {
      MPI_Recv(&data_b, 1, MPI_INT32_T, MPI_ANY_SOURCE, MPI_ANY_TAG,
               MPI_COMM_WORLD, &stat);
      switch (stat.MPI_TAG) {
      case 0:
        printf("rank 9 received data_b == %d from rank 0\n", data_b);
        break;
      case 1:
        printf("rank 9 received data_b == %d from rank 1\n", data_b);
        flag = 0;
        break;
      }
    }
  }
  MPI_Finalize();
  return 0;
}