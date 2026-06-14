/**
 * @file 2026-3-5.cpp
 * @brief MPI 数据传输及写文件
 */
#include <cstdio>
#include <cstdlib>
#include <mpi.h>

int main(int argc, char *argv[]) {
  int i, k, my_rank, nprocs;
  double data_size;
  double start_time, end_time, time_cost, average_time;
  int *p;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Status status;
  for (i = 1; i <= 1e5; i = i + 1e2) {
    p = (int *)malloc((sizeof(int) * 1000) * i);
    if (!p) {
      printf("动态分配内存失败！\n");
      exit(1);
    }
    if (my_rank == 0) {
      printf("开始传送 %d×4kb/400M 数据\n", i);
    }
    start_time = MPI_Wtime();

    for (k = 1; k <= 5; k++) {
      if (my_rank == 0) {
        MPI_Send(p, i, MPI_INT, 1, k, MPI_COMM_WORLD);
        printf("第%d回合: %d发送数据完成……\n", k, my_rank);
      }
      if (my_rank == 1) {
        MPI_Recv(p, i, MPI_INT, 0, k, MPI_COMM_WORLD, &status);
        MPI_Send(p, i, MPI_INT, 0, k, MPI_COMM_WORLD);
        printf("第%d回合:%d接收发送数据完成……\n", k, my_rank);
      }
      if (my_rank == 0) {
        MPI_Recv(p, i, MPI_INT, 1, k, MPI_COMM_WORLD, &status);
        printf("第%d回合succeed! \n", k);
      }
    }
    end_time = MPI_Wtime();
    time_cost = end_time - start_time;
    average_time = time_cost / 100;
    data_size = i / 1e3;

    FILE *fp;
    fp = fopen("experiment_data.txt", "a+");
    fprintf(fp, "%lf,%lf\n", data_size, average_time);
    fclose(fp);
    free(p);
    if (my_rank == 0) {
      printf("%f M 数据包发送接收完成 \n", data_size);
    }
    printf("来回传输一次时间为 %lf \n ", average_time);
  }
  MPI_Finalize();
  return 0;
}
