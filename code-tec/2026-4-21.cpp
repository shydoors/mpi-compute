#include <iostream>
#include <omp.h>
using namespace std;
int main() {
  int i, j, total = 0, N;
  cout << "请输入整数N：" << "\n";
  cin >> N;
#pragma omp parallel for
  for (i = 0; i <= N; i++) {
    j = omp_get_num_threads(); // 获取线程数目
    cout << "线程数目：" << j << "\n";
    j = omp_get_thread_num(); // 当前线程ID
    cout << "当前线程ID：" << j << "\n";
    // #pragma omp atomic
    total += i;
  }
  cout << "累计和为：" << total << "\n";

  return 0;
}