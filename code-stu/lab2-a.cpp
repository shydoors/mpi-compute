#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <omp.h>
int main() {
  std::int64_t n, cnt;
  double factor, omp_sum = 0.0, ser_sum = 0.0;
  n = 1000000, cnt = omp_get_num_procs();
  auto omp_start = std::chrono::high_resolution_clock::now();
// OpenMP 并行指令参数详解：
// #pragma omp parallel for : 指示编译器开启并行区域，并自动将紧随其后的 for
// 循环迭代分发给多个线程执行 num_threads(cnt)         :
// 指定并行区域中创建的线程数量 default(none)            :
// 禁用隐式数据共享规则，强制要求显式声明并行区内所有变量的作用域，提升代码安全性
// reduction(+:sum)         : 指定 sum
// 为加法归约变量。各线程使用局部副本累加，循环结束后自动合并至全局 sum
// private(factor)          : 指定 factor
// 为线程私有变量。每个线程拥有独立副本，避免多线程写入冲突（数据竞争）
// shared(n)                : 指定 n 为共享变量。所有线程共同读取同一份内存中的
// n 值，用于循环终止条件判断
#pragma omp parallel for num_threads(cnt) default(none) reduction(+ : omp_sum) private(factor) shared(n)
  for (std::int64_t i = 0; i < n; ++i) {
    factor = (i % 2 == 0) ? 1.0 : -1.0;
    omp_sum += factor / ((2 * i + 2) * (2 * i + 3) * (2 * i + 4));
  }
  omp_sum = 3.0 + 4.0 * omp_sum;
  auto omp_end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> omp_cost = omp_end - omp_start;
  auto ser_start = std::chrono::high_resolution_clock::now();
  for (std::int64_t i = 0; i < n; i++) {
    factor = (i % 2 == 0) ? 1.0 : -1.0;
    ser_sum += factor / ((2 * i + 2) * (2 * i + 3) * (2 * i + 4));
  }
  ser_sum = 3.0 + 4.0 * ser_sum;
  auto ser_end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> ser_cost = ser_end - ser_start;
  std::cout << std::fixed << std::setprecision(16);
  std::cout << "With n = " << n << " terms and " << cnt << " threads,\n";
  std::cout << ">>>> Compare of the 3 estimates" << std::endl;
  std::cout << "omp    : pi = " << omp_sum << " cost " << omp_cost.count() << std::endl;
  std::cout << "serial : pi = " << ser_sum << " cost " << ser_cost.count() << std::endl;
  std::cout << "cmath  : pi = " << 4.0 * atan(1.0) << " (4.0 * atan(1.0)) " << std::endl;
  return 0;
}
