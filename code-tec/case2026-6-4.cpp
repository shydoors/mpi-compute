#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

int main(int argc, char** argv) {
    // 1. 初始化混合并行环境
    // 声明 MPI_THREAD_FUNNELED：仅主线程调用MPI，Worker线程纯计算，符合生产安全规范
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // 2. 数据分治：每个进程管理独立的局部子域（含左右Halo区）
    const int N_LOCAL = 1000;      // 本进程负责的内部计算点
    const int HALO = 1;            // Halo宽度（依赖 stencil 模板大小）
    const int N_TOTAL = N_LOCAL + 2 * HALO;

    std::vector<double> u(N_TOTAL, 0.0);
    std::vector<double> u_new(N_TOTAL, 0.0);

    // 初始条件与物理边界
    if (rank == 0) u[0] = 100.0;               // 全局左端恒温
    if (rank == size - 1) u[N_TOTAL - 1] = 0.0; // 全局右端恒温
    for (int i = HALO; i < N_LOCAL + HALO; ++i) u[i] = 50.0; // 内部初值

    // 计算邻居秩（边界进程使用 MPI_PROC_NULL 自动忽略通信）
    int left_rank = (rank == 0) ? MPI_PROC_NULL : rank - 1;
    int right_rank = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;

    const int MAX_ITER = 500;
    const double DT = 0.24; // 简化版稳定性系数 (alpha*dt/dx^2)

    // 3. SPMD主循环：所有进程执行完全相同的代码逻辑
    for (int iter = 0; iter < MAX_ITER; ++iter) {

        // ── 步骤A：MPI跨节点 Halo 通信 ──
        // 使用 MPI_Sendrecv 避免死锁，安全交换左右边界数据
        MPI_Status status;
        MPI_Sendrecv(&u[N_LOCAL + HALO - 1], 1, MPI_DOUBLE, right_rank, 0,
            &u[0], 1, MPI_DOUBLE, left_rank, 0,
            MPI_COMM_WORLD, &status);
        MPI_Sendrecv(&u[HALO], 1, MPI_DOUBLE, left_rank, 0,
            &u[N_TOTAL - 1], 1, MPI_DOUBLE, right_rank, 0,
            MPI_COMM_WORLD, &status);

        // ── 步骤B：OpenMP 节点内细粒度并行计算 ──
        double local_max_diff = 0.0;
#pragma omp parallel for reduction(max:local_max_diff) schedule(static)
        for (int i = HALO; i < N_LOCAL + HALO; ++i) {
            // 一维热传导显式差分格式
            double laplacian = u[i - 1] - 2.0 * u[i] + u[i + 1];
            u_new[i] = u[i] + DT * laplacian;
            local_max_diff = std::max(local_max_diff, std::abs(laplacian));
        }

        // ── 步骤C：MPI 全局归约与同步 ──
        double global_max_diff = 0.0;
        MPI_Allreduce(&local_max_diff, &global_max_diff, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

        // 滚动数组，准备下一迭代
        u.swap(u_new);

        // 仅 Rank 0 输出日志（避免多进程I/O竞争）
        if (rank == 0 && iter % 50 == 0) {
            std::cout << "[Iter " << iter << "] Global Residual: " << global_max_diff << "\n";
        }

        // 全局收敛判断
        if (global_max_diff < 1e-5) break;
    }

    MPI_Finalize();
    return 0;
}