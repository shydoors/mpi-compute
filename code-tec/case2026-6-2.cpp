// functional_decomp.cpp
#include <cmath>
#include <iostream>
#include <mpi.h>
#include <omp.h>
#include <vector>

int main(int argc, char **argv) {
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  if (size < 3) {
    if (rank == 0) {
      std::cerr << "功能分解需至少 3 个 MPI 进程\n";
    }
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  const int N = 1'000'000;
  std::vector<double> buf(N);
  // 迭代耦合循环（模拟多阶段/多物理场工作流）
  for (int step = 0; step < 50; ++step) {
    if (rank == 0) {
      // 【模块A：数据预处理 / 边界生成】
#pragma omp parallel for schedule(static)
      for (int i = 0; i < N; ++i) {
        buf[i] = std::sin(i * 0.001 + step * 0.1);
      }
      MPI_Send(buf.data(), N, MPI_DOUBLE, 1, step, MPI_COMM_WORLD);
    } else if (rank == 1) {
      // 【模块B：核心求解器 / 物理计算】
      MPI_Recv(buf.data(), N, MPI_DOUBLE, 0, step, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);

#pragma omp parallel for schedule(static)
      for (int i = 0; i < N; ++i) {
        buf[i] = std::sqrt(std::abs(buf[i])) * 2.0 + std::cos(i * 0.002);
      }
      MPI_Send(buf.data(), N, MPI_DOUBLE, 2, step, MPI_COMM_WORLD);
    } else if (rank == 2) {
      // 【模块C：后处理 / 统计监控】
      MPI_Recv(buf.data(), N, MPI_DOUBLE, 1, step, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);

      double local_sum = 0.0;
#pragma omp parallel for reduction(+ : local_sum) schedule(static)
      for (int i = 0; i < N; ++i) {
        local_sum += buf[i] * buf[i];
      }
      if (step % 10 == 0) {
        std::cout << "[模块C] Step " << step << " | 统计值: " << local_sum
                  << "\n";
      }
    }
  }

  MPI_Finalize();
  return 0;
}