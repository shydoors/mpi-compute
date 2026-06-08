#include <cstdint>
#include <iostream>
#include <omp.h>
/**
 * 解决资源竞争的若干方案
 * 1. critical：临界区，串行执行
 * 2. atomic：原子操作，效率较高，但只能用于简单的读写操作
 * 3. reduction：归约操作，适用于累加、乘积等操作，效率较高，自动处理线程间的合并
 * 4. mutex：互斥锁，适用于复杂的临界区，效率较低，但提供更强的控制和灵活性
 * 5. lock_guard：基于RAII的互斥锁，自动管理锁的生命周期，避免死锁和资源泄漏
 */
int main() {
  std::cout << omp_get_num_procs() << std::endl;
  std::int32_t max_num = 100, num_t = 20;
  std::int32_t sum = 0, cnt = 0;
  std::int32_t item = max_num / num_t;
#pragma omp parallel num_threads(num_t)
  {
    std::int32_t id = omp_get_thread_num();
    cnt = omp_get_num_threads();
    for (std::int32_t i = item * id + 1;
         i <= (id == cnt ? max_num : item * (id + 1)); ++i) {
#pragma omp critical
      {
        /**
         * # pragma omp critical本质是告诉编译器：紧随其后的代码块必须互斥执行
         * 此处申明临界区,导致只能单进程访问
         * 只能保证同一时刻只有一个线程执行 sum += i 操作，避免了读-修改-写冲突
         * 但由于 sum += i 是一个简单的操作，使用 critical
         * 可能会导致性能下降，因为它会串行化所有线程的执行，无法充分利用多线程的优势
         * */
        sum += i;
        std::cout << " id = " << id << " sum = " << sum << std::endl;
      }
    }
  }
  std::cout << std::endl << sum << std::endl;
  return 0;
}