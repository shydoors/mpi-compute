// pipeline_parallel.cpp
#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <vector>
#include <cmath>

int main(int argc, char** argv) {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 3) {
        if (rank == 0) std::cerr << "流水线示例需严格 3 个进程\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    const int N = 1'000'000;      // 单块数据量
    const int CHUNKS = 8;         // 总数据块数
    std::vector<double> buf(N);

    MPI_Request req_send, req_recv;
    bool send_pending = false, recv_pending = false;

    for (int c = 0; c < CHUNKS; ++c) {
        // 1️⃣ 完成上一块的非阻塞通信（同步点）
        if (send_pending) MPI_Wait(&req_send, MPI_STATUS_IGNORE);
        if (recv_pending) MPI_Wait(&req_recv, MPI_STATUS_IGNORE);

        // 2️⃣ OpenMP 并行计算当前块（流水线排空处理）
        // rank 0 直接计算，rank>0 需等待上游数据到达后再算
        if (rank == 0 || c >= rank) {
#pragma omp parallel for schedule(static)
            for (int i = 0; i < N; ++i)
                buf[i] = std::sin(buf[i] + rank * 0.1) * 1.5; // 模拟阶段计算
        }

        // 3️⃣ 发起下一阶段非阻塞通信（实现重叠）
        if (rank < 2 && c < CHUNKS) {
            if (rank == 0) { // Stage0 额外负责生成数据
#pragma omp parallel for schedule(static)
                for (int i = 0; i < N; ++i) buf[i] = std::cos(i * 0.001 + c);
            }
            MPI_Isend(buf.data(), N, MPI_DOUBLE, rank + 1, c, MPI_COMM_WORLD, &req_send);
            send_pending = true;
        }
        if (rank > 0 && c < CHUNKS - 1) { // 提前投递下一块接收请求
            MPI_Irecv(buf.data(), N, MPI_DOUBLE, rank - 1, c + 1, MPI_COMM_WORLD, &req_recv);
            recv_pending = true;
        }
    }

    MPI_Finalize();
    return 0;
}