#include <omp.h>
#include <stdio.h>
int main() {
  double a[8], b[8], c[8];
  for (int i = 0; i < 8; i++) {
    a[i] = i;
    b[i] = i * 10.0;
  }
#pragma omp parallel for schedule(dynamic)
  for (int i = 0; i < 8; i++) {
    c[i] = a[i] + b[i];
    printf("线程 %d 计算 c[%d] = %.0f\n", omp_get_thread_num(), i, c[i]);
  }
  return 0;
}