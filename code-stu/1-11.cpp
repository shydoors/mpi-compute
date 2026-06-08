#include <cstdint>
#include <iostream>
#include <omp.h>

int main() {
  // 打印可用 CPU 核心数
  std::cout << "CPU cores: " << omp_get_num_procs() << '\n';
  constexpr std::int32_t max_num = 101; // 求和上限
  constexpr std::int32_t num_t = 32;    // 线程数
  std::int32_t sum = 0;
#pragma omp parallel for num_threads(num_t) reduction(+ : sum)
  for (std::int32_t i = 1; i <= max_num; ++i) {
    sum += i;
    // 此处不能额外加一个大括号，否则会被视为一个新的代码块，而非 for
    // 循环体的一部分
  }
  std::cout << "Sum: " << sum << '\n';
  std::int64_t ans = static_cast<int64_t>(max_num) * (max_num + 1) / 2;
  std::cout << "Answer: " << ans << '\n';
  if (ans == sum) {
    std::cout << "Correct!\n";
  }
  return 0;
}