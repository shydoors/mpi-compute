#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _OPENMP
#include <omp.h>
#endif

int main(int argc, char *argv[]) {
  int32_t num_t = strtol(argv[1], NULL, 10);
#pragma omp parallel num_threads(num_t)
  {
#ifdef _OPENMP
    int32_t rank_t = omp_get_thread_num();
#else
    int32_t rank_t = 0;
#endif
   printf("Thread %" PRId32 " says hello to you!\n", rank_t);
  }
  return 0;
}