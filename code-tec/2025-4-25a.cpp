/**
 * @file 2025-4-25a.cpp
 * @brief OpenMP 并行奇偶排序
 */
#include <iostream>
#include <omp.h>
int main() {
  bool flag = true, change = true;
  int n;
  std::cin >> n;
  int *a = new int[n];
  for (int i = 0; i < n; i++) {
    std::cin >> a[i];
  }
  omp_set_num_threads(n / 2);
  while (flag || !change) {
    flag = false;
    if (change) {
#pragma omp parallel for
      for (int i = 0; i < n - 1; i += 2) {
        if (a[i] > a[i + 1]) {
          flag = true;
          int temp = a[i];
          a[i] = a[i + 1];
          a[i + 1] = temp;
        }
      }
    } else {
#pragma omp parallel for
      for (int i = 1; i < n - 1; i += 2) {
        if (a[i] > a[i + 1]) {
          flag = true;
          int temp = a[i];
          a[i] = a[i + 1];
          a[i + 1] = temp;
        }
      }
    }
    change = !change;
  }
  for (int i = 0; i < n; i++) {
    std::cout << a[i] << " ";
  }
  delete[] a;
  return 0;
}