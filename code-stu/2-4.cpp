#include <cstdint>
#include <iostream>
#include <omp.h>
int main() {
  std::cout << omp_get_num_procs() << std::endl;
  std::int32_t sumx = 1, sumy = 2;
  omp_lock_t lock;      // 定义一个锁变量
  omp_init_lock(&lock); // 初始化锁变量
#pragma omp parallel num_threads(4) shared(sumx, sumy)
  {
    omp_set_lock(&lock); // 上锁，进入临界区
    std::cout << " <<<<<<<<<<<<<<<<<< " << std::endl;
    std::cout << "Getting in Lock Area" << std::endl;
    std::cout << "Thread ID : " << omp_get_thread_num() << std::endl;
    sumx += omp_get_thread_num();
    std::cout << "sumx ==" << sumx << std::endl;
    sumy += omp_get_thread_num();
    std::cout << "sumy ==" << sumy << std::endl;
    std::cout << "Getting out Lock Area \n" << std::endl;
    omp_unset_lock(&lock); // 解锁，离开临界区
  }
  omp_destroy_lock(&lock); // 销毁锁变量
  std::cout << "After Parallel Region :" << std::endl;
  std::cout << "sumx ==" << sumx << "  sumy ==" << sumy << std::endl;
  return 0;
}