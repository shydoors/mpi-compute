#include <chrono>
#include <iomanip>
#include <iostream>
#include <omp.h>
#include <string>
#include <thread>

// 线程安全日志输出
void safe_log(int tid, const std::string &msg) {
#pragma omp critical
  std::cout << "[T" << tid << "] " << msg << std::endl;
}

// 模拟耗时任务（毫秒）
void busy_wait(int ms) {
  auto start = std::chrono::high_resolution_clock::now();
  while (std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::high_resolution_clock::now() - start)
             .count() < ms) {
    // 忙等待，确保时间精确
  }
}

// =========================================================
// 演示 1：master vs single vs single(nowait) 的屏障影响
// =========================================================
void demo_barrier_control() {
  const int NUM_THREADS = 4;
  const int PHASE_MS = 100;
  std::cout << "\n========== 演示 1：隐式屏障对比 ==========\n";
  // 1.1 master：末尾【无】隐式屏障
  double t_start = omp_get_wtime();
#pragma omp parallel num_threads(NUM_THREADS)
  {
    int tid = omp_get_thread_num();
    busy_wait(PHASE_MS); // 阶段1
    safe_log(tid, "阶段1完成");
#pragma omp master
    {
      safe_log(tid, "进入 MASTER 块 (仅T0执行)");
      busy_wait(PHASE_MS * 2); // 模拟较重任务
      safe_log(tid, "MASTER 块完成");
    }
    // 注意：master 结束后，其他线程【不会等待】，直接继续
    busy_wait(PHASE_MS); // 阶段2
    safe_log(tid, "阶段2完成");
  }
  std::cout << "[master] 总耗时: " << std::fixed << std::setprecision(3)
            << (omp_get_wtime() - t_start)
            << "s (预期 ~0.2s，阶段2与master并行)\n";

  // 1.2 single：末尾【有】隐式屏障
  t_start = omp_get_wtime();
#pragma omp parallel num_threads(NUM_THREADS)
  {
    int tid = omp_get_thread_num();
    busy_wait(PHASE_MS);
    safe_log(tid, "阶段1完成");
#pragma omp single
    {
      safe_log(tid, "进入 SINGLE 块 (任意1线程执行)");
      busy_wait(PHASE_MS * 2);
      safe_log(tid, "SINGLE 块完成");
    }
    // 注意：single 结束后，所有线程【必须等待】屏障同步
    busy_wait(PHASE_MS);
    safe_log(tid, "阶段2完成");
  }
  std::cout << "[single] 总耗时: " << std::fixed << std::setprecision(3)
            << (omp_get_wtime() - t_start)
            << "s (预期 ~0.3s，阶段2被屏障阻塞)\n";

  // 1.3 single nowait：显式取消屏障
  t_start = omp_get_wtime();
#pragma omp parallel num_threads(NUM_THREADS)
  {
    int tid = omp_get_thread_num();
    busy_wait(PHASE_MS);

#pragma omp single nowait
    {
      busy_wait(PHASE_MS * 2);
      safe_log(tid, "SINGLE(nowait) 完成");
    }
    // 注意：nowait 取消屏障，阶段2立即开始
    busy_wait(PHASE_MS);
  }
  std::cout << "[single nowait] 总耗时: " << std::fixed << std::setprecision(3)
            << (omp_get_wtime() - t_start) << "s (预期 ~0.2s，无阻塞)\n";
}

// =========================================================
// 演示 2：masked 的屏障与过滤行为 (需 OpenMP 5.1+)
// =========================================================
void demo_masked_barrier() {
  const int NUM_THREADS = 6;
  std::cout << "\n========== 演示 2：masked 过滤与屏障 ==========\n";
  std::cout << "(注：需编译器支持 OpenMP 5.1，否则跳过)\n";

#if _OPENMP >= 202011
  double t_start = omp_get_wtime();
#pragma omp parallel num_threads(NUM_THREADS)
  {
    int tid = omp_get_thread_num();
    bool eligible = (tid < 3); // 仅 T0,T1,T2 有资格竞争

#pragma omp masked filter(eligible)
    {
      safe_log(tid, "进入 MASKED 块 (过滤条件: tid<3)");
      busy_wait(150);
      safe_log(tid, "MASKED 块完成");
    }
    // masked 末尾【有】隐式屏障，所有线程等待块完成
    safe_log(tid, "通过屏障，继续执行");
  }
  std::cout << "[masked] 总耗时: " << std::fixed << std::setprecision(3)
            << (omp_get_wtime() - t_start) << "s (屏障同步，行为同 single)\n";
#else
  std::cout << "编译器不支持 OpenMP 5.1，masked 演示已跳过。\n";
#endif
}

// =========================================================
// 演示 3：atomic vs omp_lock_t 的数据同步（无屏障）
// =========================================================
void demo_data_sync_no_barrier() {
  const int NUM_THREADS = 4;
  const long long ITER = 5000000;

  std::cout << "\n========== 演示 3：atomic vs lock (无隐式屏障) ==========\n";

  // 3.1 atomic：硬件原子指令，无屏障
  long long counter = 0;
  double t_start = omp_get_wtime();
#pragma omp parallel for num_threads(NUM_THREADS) schedule(static)
  for (long long i = 0; i < ITER; ++i) {
#pragma omp atomic
    counter++;
  }
  std::cout << "[atomic] 耗时: " << std::fixed << std::setprecision(3)
            << (omp_get_wtime() - t_start) << "s | 结果: " << counter << "\n";

  // 3.2 omp_lock_t：软件互斥锁，无屏障
  counter = 0;
  omp_lock_t lock;
  omp_init_lock(&lock);
  t_start = omp_get_wtime();
#pragma omp parallel for num_threads(NUM_THREADS) schedule(static)
  for (long long i = 0; i < ITER; ++i) {
    omp_set_lock(&lock);
    counter++;
    omp_unset_lock(&lock);
  }
  omp_destroy_lock(&lock);
  std::cout << "[lock]   耗时: " << std::fixed << std::setprecision(3)
            << (omp_get_wtime() - t_start) << "s | 结果: " << counter << "\n";

  std::cout
      << "说明: atomic 和 lock 仅保证 counter "
         "读写安全，【不产生任何隐式屏障】。\n"
      << "      线程独立推进循环，仅在临界区排队。总耗时取决于最慢线程。\n";
}

int main() {
  std::cout << "OpenMP 屏障与同步演示程序\n"
            << "编译: g++ -fopenmp -O2 demo.cpp -o ../build/a\n"
            << "运行: ../build/a\n";

  demo_barrier_control();
  demo_masked_barrier();
  demo_data_sync_no_barrier();
  return 0;
}