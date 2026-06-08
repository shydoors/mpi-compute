#include <mpi.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>
#include <algorithm>

/**
 * @brief 本地一维卷积计算
 */
std::vector<int> localConvolve(const std::vector<int>& local_signal,
    const std::vector<int>& kernel,
    int local_output_size) {
    std::vector<int> local_output(local_output_size, 0);
    int kernel_size = static_cast<int>(kernel.size());
    int halo = kernel_size - 1;
    int local_signal_size = static_cast<int>(local_signal.size());

    for (int out_idx = 0; out_idx < local_output_size; ++out_idx) {
        int sum = 0;
        for (int k = 0; k < kernel_size; ++k) {
            int sig_idx = out_idx + k;  // local_signal 中的索引
            if (sig_idx >= 0 && sig_idx < local_signal_size) {
                sum += local_signal[sig_idx] * kernel[kernel_size - 1 - k];
            }
        }
        local_output[out_idx] = sum;
    }
    return local_output;
}

int main(int argc, char** argv) {
    // ========== 配置参数 ==========
    constexpr int GLOBAL_SIGNAL_LENGTH = 200000;
    constexpr int KERNEL_LENGTH = 21;
    constexpr unsigned int RANDOM_SEED = 42;
    // =============================

    MPI_Init(&argc, &argv);

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    int halo = KERNEL_LENGTH - 1;
    int output_length = GLOBAL_SIGNAL_LENGTH + halo;

    // ========== 根进程准备数据 ==========
    std::vector<int> global_signal;
    std::vector<int> kernel(KERNEL_LENGTH);

    if (world_rank == 0) {
        // 生成信号
        global_signal.resize(GLOBAL_SIGNAL_LENGTH);
        std::mt19937 gen(RANDOM_SEED);
        std::uniform_int_distribution<> dis(-50, 49);
        for (int& val : global_signal) val = dis(gen);

        // 生成卷积核
        int third = KERNEL_LENGTH / 3;
        for (int i = 0; i < KERNEL_LENGTH; ++i) {
            kernel[i] = (i < third) ? 1 : (i < 2 * third) ? 0 : -1;
        }

        std::cout << "===== MPI 并行卷积（修复版）=====\n";
        std::cout << "全局信号长度：" << GLOBAL_SIGNAL_LENGTH << "\n";
        std::cout << "卷积核长度：" << KERNEL_LENGTH << "\n";
        std::cout << "进程数量：" << world_size << "\n";
        std::cout << "输出长度：" << output_length << "\n\n";
    }

    // ========== 广播卷积核 ==========
    MPI_Bcast(kernel.data(), KERNEL_LENGTH, MPI_INT, 0, MPI_COMM_WORLD);

    // ========== 计算数据划分（所有进程一致）==========
    // 输出空间均匀划分
    int base_output = output_length / world_size;
    int remainder = output_length % world_size;

    int local_output_len = base_output + (world_rank < remainder ? 1 : 0);
    int local_output_start = world_rank * base_output + std::min(world_rank, remainder);

    // 计算需要的信号范围（含 halo）
    int sig_start = std::max(0, local_output_start - halo);
    int sig_end = std::min(GLOBAL_SIGNAL_LENGTH, local_output_start + local_output_len);
    int local_signal_len = sig_end - sig_start + halo;  // 右侧 halo

    // ========== 根进程准备 Scatterv 参数 ==========
    std::vector<int> send_counts(world_size);
    std::vector<int> send_displs(world_size);
    std::vector<int> recv_counts(world_size);
    std::vector<int> recv_displs(world_size);

    if (world_rank == 0) {
        int current_displ = 0;
        for (int r = 0; r < world_size; ++r) {
            // 每个进程的输出范围
            int r_output_len = base_output + (r < remainder ? 1 : 0);
            int r_output_start = r * base_output + std::min(r, remainder);

            // 计算该进程需要的信号范围
            int r_sig_start = std::max(0, r_output_start - halo);
            int r_sig_end = std::min(GLOBAL_SIGNAL_LENGTH, r_output_start + r_output_len);
            int r_signal_len = r_sig_end - r_sig_start + halo;

            send_counts[r] = r_signal_len;
            send_displs[r] = r_sig_start;

            // 用于验证
            recv_counts[r] = r_signal_len;
            recv_displs[r] = current_displ;
            current_displ += r_signal_len;
        }

        // 打印划分信息（调试用）
        std::cout << "数据划分详情:\n";
        for (int r = 0; r < world_size; ++r) {
            std::cout << "  进程 " << r << ": 信号 [" << send_displs[r]
                << ", " << send_displs[r] + send_counts[r] - 1 << "]"
                << " 长度=" << send_counts[r]
                << " 输出 [" << (r * base_output + std::min(r, remainder))
                << ", " << (r * base_output + std::min(r, remainder) + base_output + (r < remainder ? 1 : 0) - 1) << "]\n";
        }
        std::cout << "\n";
    }

    // ========== 分发信号段 ==========
    std::vector<int> local_signal(local_signal_len);
    MPI_Scatterv(global_signal.data(), send_counts.data(), send_displs.data(), MPI_INT,
        local_signal.data(), local_signal_len, MPI_INT,
        0, MPI_COMM_WORLD);

    // ========== 并行计算（计时）==========
    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();

    auto local_output = localConvolve(local_signal, kernel, local_output_len);

    MPI_Barrier(MPI_COMM_WORLD);
    double end_time = MPI_Wtime();
    double elapsed = end_time - start_time;

    // ========== 收集结果 ==========
    std::vector<int> out_counts(world_size), out_displs(world_size);
    if (world_rank == 0) {
        int current_displ = 0;
        for (int r = 0; r < world_size; ++r) {
            out_counts[r] = base_output + (r < remainder ? 1 : 0);
            out_displs[r] = current_displ;
            current_displ += out_counts[r];
        }
    }
    std::vector<int> global_output;
    if (world_rank == 0) {
        global_output.resize(output_length);
    }
    MPI_Gatherv(local_output.data(), local_output_len, MPI_INT,
        global_output.data(), out_counts.data(), out_displs.data(), MPI_INT,
        0, MPI_COMM_WORLD);

    // ========== 输出结果 ==========
    if (world_rank == 0) {
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "===== 计算完成 =====\n";
        std::cout << "并行运行时间：" << elapsed << " 秒\n";
        std::cout << "输出长度：" << global_output.size() << "\n";

        std::cout << "\n输出前 10 个值：";
        for (int i = 0; i < 10 && i < static_cast<int>(global_output.size()); ++i) {
            std::cout << global_output[i] << " ";
        }
        std::cout << "...\n";

        std::cout << "输出后 10 个值：";
        for (size_t i = global_output.size() - 10; i < global_output.size(); ++i) {
            std::cout << global_output[i] << " ";
        }
        std::cout << "\n";
    }
    MPI_Finalize();
    return 0;
}