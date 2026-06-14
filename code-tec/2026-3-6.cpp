/**
 * @file 2026-3-6.cpp
 * @brief MPI 求曲线面积（梯形法求定积分）
 */
#include "mpi.h"
#include <cmath>
#include <iostream>
double fun(double x) { return pow(x, 2); }
double Trap(double left_endpt, double right_endpt, int trap_count,
            double base_len) {
  double estimate, x;
  estimate = (fun(left_endpt) + fun(right_endpt)) / 2.0;
  for (int i = 1; i <= trap_count; i++) {
    x = left_endpt + i * base_len;
    estimate += fun(x);
  }
  estimate = estimate * base_len;
  return estimate;
}

int main(int argc, char *argv[]) {
  int my_rank, comm_sz, n, local_n, source;
  double a, b, h, local_a, local_b, local_int, total_int;
  a = 0;
  b = 10.0;
  n = 1000000000;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

  double t1 = MPI_Wtime();
  h = (b - a) / (double)n;
  local_n = n / comm_sz;
  local_a = a + my_rank * local_n * h;
  local_b = local_a + local_n * h;
  local_int = Trap(local_a, local_b, local_n, h);

  if (my_rank != 0) {
    MPI_Send(&local_int, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
  } else {
    total_int = local_int;
    for (source = 1; source < comm_sz; source++) {
      MPI_Recv(&local_int, 1, MPI_DOUBLE, source, 0, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
      total_int += local_int;
    }
  }
  if (my_rank == 0) {
    std::cout << "\n结果是:" << total_int << std::endl;
    double t2 = MPI_Wtime();
    std::cout << "\n时间长度是:" << t2 - t1 << std::endl;
  }

  MPI_Finalize();
  return 0;
}
