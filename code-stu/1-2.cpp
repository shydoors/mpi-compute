#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <omp.h>
int main(int argc, char *argv[]) {
  std::int32_t num_t = std::strtol(argv[1], nullptr, 10);
#pragma omp parallel num_threads(num_t)
  {
    std::int32_t rank_t = omp_get_thread_num();
    // std::cout << " Thread " << rank_t << " says hello\n" << std::endl;
    printf(" Thread  %" PRId32 " ,says hello \n", rank_t);
  }
  return 0;
}