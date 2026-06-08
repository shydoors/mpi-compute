/*
 * 对 f=x^2 在 [0,1] 区间进行积分
 */
#include <mpi.h>
#include <stdio.h>
double f(double x) { return x * x; } // 被积函数
double local_trapezoidal(double a, double b, int n) {
  double h = (b - a) / n;
  double sum = (f(a) + f(b)) / 2.0;
  for (int i = 1; i < n; i++) {
    sum += f(a + i * h);
  }
  return sum * h;
}
int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  if (rank == 0) {
    printf("目标：对f(x)=x^2在 [0,1]区间进行积分\n");
    printf("启动进程数: %d\n", size);
  }
  double a = 0.0, b = 1.0;
  int n_local = 10000;
  double local_a = a + rank * (b - a) / size;
  double local_b = a + (rank + 1) * (b - a) / size;
  double local_result = local_trapezoidal(local_a, local_b, n_local);
  if (rank == 0) {
    double total = local_result;
    double recv_val;
    // 点对点接收其他进程的结果
    for (int i = 1; i < size; i++) {
      MPI_Recv(&recv_val, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
      total += recv_val;
    }
    printf("[梯形积分结果]: %.6f \n", total);
  } else {
    // 点对点发送局部结果给0号进程
    MPI_Send(&local_result, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
  }
  MPI_Finalize();
  return 0;
}