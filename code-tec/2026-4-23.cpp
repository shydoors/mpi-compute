#include <iostream>
#include <omp.h>
using namespace std;
int main() {
  int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  int i, j, b;
#pragma omp parallel for private(b) ordered
  for (i = 0; i < 10; i++) {
    b = a[i];
#pragma omp ordered
    cout << "线程数目：" << omp_get_num_threads() << " 当前线程ID："
         << omp_get_thread_num() << "\n";
    a[i] = a[i] + b;
  }
  cout << "\n计算结果为：\n";
  for (i = 0; i < 10; i++) {
    cout << "a[" << i << "]:" << a[i] << "\n";
  }
  return 0;
}