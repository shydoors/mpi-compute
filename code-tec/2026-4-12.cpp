#include <iostream>
#include <omp.h>
using namespace std;
int main() {
  cout << "reduction输出：\n";
  int i = 0, j = 10;
#pragma omp parallel for reduction(* : j)
  for (i = 0; i < 8; i++) {
    j = 2;
    cout << i << ":" << j << "\n";
    j++;
    cout << i << ":" << j << "\n";
  }
  cout << "最后j的值为:" << j << "\n";
  return 0;
}