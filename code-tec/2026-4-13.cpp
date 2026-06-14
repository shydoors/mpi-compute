#include <iostream>
#include <omp.h>
using namespace std;
const int Count = 100;
int main() {
  int sum = 100; // Assign an initial value.
#pragma omp parallel for reduction(+ : sum)
  for (int i = 0; i < Count; i++) {
    sum += i;
  }
  cout << "Sum: " << sum << endl;
  return 0;
}