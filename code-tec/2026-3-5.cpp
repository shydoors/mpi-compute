// 数据的传输及写文件
#include <mpi.h>    //mpi的接口
#include <stdio.h>  //标准输入输出头文件
#include <stdlib.h> //标准库
int main(int argc, char *argv[]) {
  int i, j, k, my_rank, nprocs;
  double data_size;
  double start_time, end_time, time_cost, average_time;
  int *p;                 // 动态分配内存,存放int型数据 1个int 4个字节（Byte）
  MPI_Init(&argc, &argv); // 启动并行环境
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);  // 获取总进程数
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); // 获取本地进程编号
  MPI_Status status;
  for (i = 1; i <= 1e5; i = i + 1e2) {
    p = (int *)malloc((sizeof(int) * 1000) *
                      i); // 分配i个内存空间，每个大小为sizeof(int)*1000
                          // 即4000B=4kb, 分配的内存内随机赋值
    if (!p) {
      printf("动态分配内存失败！\n");
      exit(1);
    }
    if (my_rank == 0) {
      printf("开始传送 %d×4kb/400M 数据\n", i);
    }

    start_time = MPI_Wtime(); // 获取墙上时间
    // 来回传输100次
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
        // MPI_Send(p, i, MPI_INT, 1, k,
        // MPI_COMM_WORLD);//不能再MPI_Send,与上面突，死锁。
        printf("第%d回合succeed! \n", k);
      }
    }
    end_time = MPI_Wtime();
    time_cost = end_time - start_time;
    average_time = time_cost / 100;
    data_size = i / 1e3; // 转换为M为单位

    // 创建experiment_data.txt文件，将数据写入
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

  MPI_Finalize(); // 结束并行环境
  return 0;
}
