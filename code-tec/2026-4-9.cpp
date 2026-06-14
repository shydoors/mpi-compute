#include <iostream>
#include <omp.h>
using namespace std;
int main() {
  cout << "private输出：\n";
  int i = 0, j = 10;
#pragma omp parallel for private(j)
  for (i = 0; i < 8; i++) {
    cout << i << ":" << j << "\n";
    j++;
    cout << i << ":" << j << "\n";
  }
  return 0;
}
