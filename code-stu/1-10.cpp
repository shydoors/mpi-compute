#include <cstdint>
#include <iostream>
#include <omp.h>
int main() {
  constexpr std::int32_t max_num = 100; // 求和上限
  constexpr std::int32_t num_t = 32;    // 线程数
  std::int32_t sum = 0;
  const std::int32_t item = max_num / num_t;
  // 打印可用 CPU 核心数
  std::cout << "CPU cores: " << omp_get_num_procs() << '\n';
#pragma omp parallel num_threads(num_t) reduction(+ : sum)
  {
    std::int32_t id = omp_get_thread_num();
    std::int32_t cnt = omp_get_num_threads();
    // 计算当前线程处理的起始和结束索引
    std::int32_t start = item * id + 1;
    std::int32_t end = (id == cnt - 1) ? max_num : (item * (id + 1));
    for (std::int32_t i = start; i <= end; ++i) {
      sum += i;
    }
  }
  // 输出并行求和结果
  std::cout << "Sum: " << sum << '\n';
  // 公式计算的正确答案（注意用 64 位避免溢出）
  int64_t ans = static_cast<int64_t>(max_num) * (max_num + 1) / 2;
  std::cout << "Answer: " << ans << '\n';
  if (ans == sum) {
    std::cout << "Correct!\n";
  }
  return 0;
}