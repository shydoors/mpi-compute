#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <omp.h>

int main() {
  std::cout << "CPU cores: " << omp_get_num_procs() << std::endl;
  int32_t max_num = 100, num_t = 32;
  int32_t sum = 0;
  int32_t item = max_num / num_t;

#pragma omp parallel num_threads(num_t) default(none)                          \
    shared(sum, max_num, num_t, item) private(localsum)
  {
    int32_t id = omp_get_thread_num();
    int32_t cnt = omp_get_num_threads();
    int32_t localsum = 0;
    for (int32_t i = item * id + 1;
         i <= (id == cnt - 1 ? max_num : item * (id + 1)); ++i) {
      localsum += i;
    }

#pragma omp critical
    {
      sum += localsum;
    }
  }

  std::cout << "Final sum: " << sum << std::endl;
  return 0;
}