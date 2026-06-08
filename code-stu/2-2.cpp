#include <cstdint>
#include <iostream>
#include <omp.h>
int main() {
  std::cout << " prama 2-2" << std::endl;
  int32_t sum = 0, max_num;
  std::cin >> max_num;
#pragma omp parallel for
  for (int i = 0; i < max_num; ++i) {
#pragma omp atomic
    sum += i;

    // atomic需要保证在当前线程完成操作之前，其它线程不允许操作
    // 和critical的区别是，atomic只能保证单个变量的原子操作，而critical可以保护一段代码块，允许多个变量的原子操作。
  }
  std::cout << "sum = " << sum << std::endl;
  return 0;
}