#include <stdio.h>
#ifdef _OPENMP
#include <omp.h>
#endif
int main() {
#ifdef _OPENMP
  printf(" OpenMP支持 | 版本代码: %ld\n", (long)_OPENMP);
  printf(" 可用物理核数: %d\n", omp_get_num_procs());
#else
  printf("编译器不支持OpenMP\n");
  return 1;
#endif
  printf(" 并行区外调用 omp_get_num_threads() 返回: %d\n",
         omp_get_num_threads());
#pragma omp parallel num_threads(4)
  {
    int tid = omp_get_thread_num(); // 当前线程编号 (0~3)
    int n = omp_get_num_threads();  // 当前线程组总线程数 (固定为4)
    if (tid == 0)
      printf("[主线程] ID=%d | 团队大小=%d\n", tid, n);
    else
      printf("[工作线程] ID=%d | 团队大小=%d\n", tid, n);
  }
  return 0;
}