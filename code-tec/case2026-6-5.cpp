#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <vector>
#include <queue>
#include <cmath>

// 数据结构定义
struct Task { int id; int workload; };
struct Result { int task_id; double computed_value; };

// MPI 通信标签
const int MASTER_RANK = 0;
const int TAG_WORK = 1;
const int TAG_RESULT = 2;
const int TAG_DONE = 3;

// ───────────────── Worker 逻辑 ─────────────────
void run_worker() {
    while (true) {
        Task task;
        MPI_Status status;
        // 阻塞接收任务（生产环境可替换为 MPI_Irecv 实现异步监听）
        MPI_Recv(&task, sizeof(Task), MPI_BYTE, MASTER_RANK, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == TAG_DONE) break; // 收到终止信号退出

        // OpenMP 节点内细粒度并行：模拟高开销不规则计算
        double sum = 0.0;
#pragma omp parallel for reduction(+:sum) schedule(dynamic)
        for (int i = 0; i < task.workload; ++i) {
            sum += std::sin(i * 0.001) * std::cos(i * 0.002); // 计算密集型伪负载
        }

        Result res{ task.id, sum };
        // 异步回传结果（生产环境建议使用 MPI_Isend + 双缓冲）
        MPI_Send(&res, sizeof(Result), MPI_BYTE, MASTER_RANK, TAG_RESULT, MPI_COMM_WORLD);
    }
}

// ───────────────── Master 逻辑 ─────────────────
void run_master(int world_size) {
    // 1. 初始化不规则任务池（模拟负载不均场景）
    std::vector<Task> tasks;
    for (int i = 0; i < 40; ++i) {
        tasks.push_back({ i, 500000 + i * 200000 }); // 计算量呈阶梯式增长
    }
    std::queue<int> idle_workers;
    for (int i = 1; i < world_size; ++i){ idle_workers.push(i);}
    int dispatched = 0;
    int completed = 0;
    const int total_tasks = tasks.size();

    // 2. 初始批次分发（填满空闲 Worker）
    while (!idle_workers.empty() && dispatched < total_tasks) {
        int w = idle_workers.front(); idle_workers.pop();
        MPI_Send(&tasks[dispatched], sizeof(Task), MPI_BYTE, w, TAG_WORK, MPI_COMM_WORLD);
        dispatched++;
    }
    // 3. 动态调度循环：请求-响应驱动，无全局屏障
    while (completed < total_tasks) {
        Result res;
        MPI_Status status;
        MPI_Recv(&res, sizeof(Result), MPI_BYTE, MPI_ANY_SOURCE, TAG_RESULT, MPI_COMM_WORLD, &status);
        std::cout << "[Master] 任务 " << res.task_id
            << " 完成 | 来源: Rank " << status.MPI_SOURCE
            << " | 耗时负载: " << tasks[res.task_id].workload << "\n";
        completed++;

        // 4. 即时派发新任务（保持流水线流动）
        if (dispatched < total_tasks) {
            int w = status.MPI_SOURCE;
            MPI_Send(&tasks[dispatched], sizeof(Task), MPI_BYTE, w, TAG_WORK, MPI_COMM_WORLD);
            dispatched++;
        }
        else {
            idle_workers.push(status.MPI_SOURCE); // 任务池耗尽，Worker 标记为空闲
        }
    }

    // 5. 发送终止信号
    for (int i = 1; i < world_size; ++i) {
        Task term = { -1, 0 };
        MPI_Send(&term, sizeof(Task), MPI_BYTE, i, TAG_DONE, MPI_COMM_WORLD);
    }
    std::cout << "[Master] 所有任务处理完毕，流水线安全退出。\n";
}

// ───────────────── 主入口 ─────────────────
int main(int argc, char** argv) {
    int provided;
    // 声明 MPI_THREAD_SERIALIZED：允许多线程调用 MPI（需加锁），符合生产规范
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size < 2) {
        if (rank == MASTER_RANK) std::cerr << "错误：至少需要 1 Master + 1 Worker\n";
        MPI_Finalize(); return 1;
    }
    if (rank == MASTER_RANK){ run_master(size);}
    else{ run_worker();}

    MPI_Finalize();
    return 0;
}