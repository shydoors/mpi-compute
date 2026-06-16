#include <iostream>
#include <omp.h>
int main() {
  std::cout << " Program 2-1 " << std::endl;
#pragma omp parallel num_threads(4)
  {
    printf("thread %d is running \n", omp_get_thread_num());
// #pragma omp master
#pragma omp masked filter(0)
    {
      // 标准规范升级带来的弃用警告（Deprecation Warning）
      printf(">>>>>>>>>>>>>>>>[master] thread %d in line %d \n",
             omp_get_thread_num(), __LINE__);
    }
#pragma omp single
    {
      printf("[single] thread %d in line %d \n", omp_get_thread_num(),__LINE__);
    }
  }
  return 0;
}
/**
 * __LINE__
 * 是一个预定义的宏，在编译时会被替换为当前行号的整数值。在上面的代码中，__LINE__
 * 会被替换为对应的行号，输出结果会显示每个线程在执行到该行时的行号。
 *
 * single和master的区别在于：
 * -
 * master：只有主线程（线程编号为0）会执行master块中的代码，而其他线程会跳过该块。这意味着只有主线程会输出master相关的信息，类似main函数中的代码。
 * -
 * single：只有一个线程会执行single块中的代码，但这个线程不一定是主线程。OpenMP会自动选择一个线程来执行single块中的代码，而其他线程会跳过该块。这意味着可能会有多个线程输出single相关的信息，但每次运行时可能是不同的线程，类似互斥锁。
 */