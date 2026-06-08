// hybrid_domain_decomp.cpp
#include "mpi.h"
#include <omp.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

int main(int argc, char** argv) {
    int provided;
    // 1. 初始化：声明仅需主线程调用MPI (最稳定模式)
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    if (provided < MPI_THREAD_FUNNELED) {
        std::cerr << "MPI implementation does not support MPI_THREAD_FUNNELED\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int N = 2048;          // 全局网格尺寸 N x N
    const int max_iter = 2000;
    const double tol = 1e-5;

    // 2. MPI 粗粒度划分：沿 Y 轴切分为 size 个条带
    int rows_per_proc = N / size;
    int remainder = N % size;
    int local_rows = rows_per_proc + (rank < remainder ? 1 : 0);

    // 邻居进程（边界自动映射为 MPI_PROC_NULL，Sendrecv 会安全跳过）
    int up_rank = (rank == 0) ? MPI_PROC_NULL : rank - 1;
    int down_rank = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;

    // 3. 分配局部内存：含上下各 1 行 Halo 区
    // 布局: [Halo_up][Local_1..Local_rows][Halo_down] -> 总行数 local_rows+2
    int local_dim_r = local_rows + 2;
    std::vector<double> u(local_dim_r * N, 0.0);
    std::vector<double> u_new(local_dim_r * N, 0.0);

    // 设置全局 Dirichlet 边界条件
    if (rank == 0) {
        for (int c = 1; c < N - 1; ++c) u[1 * N + c] = 100.0; // 全局顶边
    }
    if (rank == size - 1) {
        for (int c = 1; c < N - 1; ++c) u[local_rows * N + c] = 0.0; // 全局底边
    }

    double diff = 1.0;
    int iter = 0;

    while (iter < max_iter && diff > tol) {
        // ================= MPI 层：跨节点 Halo 交换 =================
        MPI_Sendrecv(&u[1 * N], N, MPI_DOUBLE, up_rank, 0,
            &u[0 * N], N, MPI_DOUBLE, up_rank, 1,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&u[local_rows * N], N, MPI_DOUBLE, down_rank, 0,
            &u[(local_rows + 1) * N], N, MPI_DOUBLE, down_rank, 1,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // ================= OpenMP 层：节点内细粒度计算 =================
        double local_max_diff = 0.0;
#pragma omp parallel for reduction(max:local_max_diff) schedule(static)
        for (int r = 1; r <= local_rows; ++r) {
            for (int c = 1; c < N - 1; ++c) {
                u_new[r * N + c] = 0.25 * (u[(r - 1) * N + c] + u[(r + 1) * N + c] +
                    u[r * N + c - 1] + u[r * N + c + 1]);
                double d = std::abs(u_new[r * N + c] - u[r * N + c]);
                if (d > local_max_diff) local_max_diff = d;
            }
        }

        // ================= MPI 层：全局收敛同步 =================
        MPI_Allreduce(&local_max_diff, &diff, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

        // 更新解
        u.swap(u_new);
        iter++;

        if (rank == 0 && iter % 200 == 0) {
            std::cout << "[Rank 0] Iter " << iter << ", Global Max Diff: " << diff << "\n";
        }
    }

    MPI_Finalize();
    return 0;
}