#include <mpi.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <ctime>

#define ARRAY_SIZE 100
// NUM_PROCESSES 仅为提示，实际进程数由 MPI 决定

int main(int argc, char** argv) {
    int rank, size;
    std::vector<double> local_array(ARRAY_SIZE);
    std::vector<double> global_array(ARRAY_SIZE);
    double start_time, end_time;

    // 初始化 MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // 检查进程数
    if (size < 2) {
        if (rank == 0) {
            std::cout << "警告：需要至少 2 个进程才能演示归约操作" << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    // 初始化随机数种子（每个进程不同）
    std::srand(static_cast<unsigned>(std::time(nullptr)) + rank * 1000);

    // ========================================
    // 场景 1：初始化本地数组
    // ========================================
    std::cout << "进程 " << rank << ": 初始化本地数组" << std::endl;
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        // 每个进程生成不同的数据
        local_array[i] = (rank + 1) * 10.0 + i * 0.1 + (std::rand() % 100) * 0.01;
    }

    // 显示部分本地数据
    if (rank == 0) {
        std::cout << "\n=== 本地数组示例（进程 0 前 5 个元素）===\n";
        std::cout << std::fixed << std::setprecision(4);
        for (int i = 0; i < 5; ++i) {
            std::cout << "local_array[" << i << "] = " << local_array[i] << std::endl;
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);  // 同步所有进程

    // ========================================
    // 场景 2：数组求和归约
    // ========================================
    start_time = MPI_Wtime();

    MPI_Reduce(local_array.data(), global_array.data(), ARRAY_SIZE, MPI_DOUBLE,
        MPI_SUM, 0, MPI_COMM_WORLD);

    end_time = MPI_Wtime();

    if (rank == 0) {
        std::cout << "\n=== 数组求和归约结果 ===\n";
        std::cout << "归约耗时：" << std::setprecision(6) << end_time - start_time << " 秒\n";
        std::cout << "前 5 个元素的归约结果:\n";
        for (int i = 0; i < 5; ++i) {
            std::cout << "global_array[" << i << "] = " << global_array[i] << std::endl;
        }

        // 验证结果（忽略随机部分）
        std::cout << "\n验证：每个元素应该是各进程对应元素之和\n";
        std::cout << "理论值 (进程 0 前 5 个元素之和，忽略随机部分): ";
        for (int i = 0; i < 5; ++i) {
            double expected = 0.0;
            for (int p = 0; p < size; ++p) {
                expected += (p + 1) * 10.0 + i * 0.1;  // 忽略随机部分
            }
            std::cout << expected << " ";
        }
        std::cout << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // ========================================
    // 场景 3：数组最大值归约
    // ========================================
    std::fill(global_array.begin(), global_array.end(), 0.0);

    MPI_Reduce(local_array.data(), global_array.data(), ARRAY_SIZE, MPI_DOUBLE,
        MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "\n=== 数组最大值归约结果 ===\n";
        std::cout << "前 5 个元素的最大值:\n";
        for (int i = 0; i < 5; ++i) {
            std::cout << "global_array[" << i << "] = " << global_array[i] << std::endl;
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // ========================================
    // 场景 4：数组最小值归约
    // ========================================
    std::fill(global_array.begin(), global_array.end(), 0.0);

    MPI_Reduce(local_array.data(), global_array.data(), ARRAY_SIZE, MPI_DOUBLE,
        MPI_MIN, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "\n=== 数组最小值归约结果 ===\n";
        std::cout << "前 5 个元素的最小值:\n";
        for (int i = 0; i < 5; ++i) {
            std::cout << "global_array[" << i << "] = " << global_array[i] << std::endl;
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // ========================================
    // 场景 5：部分数组归约（只归约前 50 个元素）
    // ========================================
    int partial_count = 50;
    std::vector<double> partial_result(partial_count);

    MPI_Reduce(local_array.data(), partial_result.data(), partial_count, MPI_DOUBLE,
        MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "\n=== 部分数组归约（前 " << partial_count << " 个元素）===\n";
        std::cout << "partial_result[0] = " << partial_result[0] << std::endl;
        std::cout << "partial_result[" << partial_count - 1 << "] = " << partial_result[partial_count - 1] << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // ========================================
    // 清理
    // ========================================
    MPI_Finalize();
    return 0;
}