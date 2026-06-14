#include <iostream>
#include <omp.h>
using namespace std;
// 函数fx
int main() {
  cout << "firstprivate输出：\n";
  int i = 0, j = 10;
#pragma omp parallel for firstprivate(j)
  for (i = 0; i < 8; i++) {
    cout << i << ":" << j << "\n";
    j++;
    cout << i << ":" << j << "\n";
  }
  cout << i << ":" << j << "\n";
  return 0;
}
