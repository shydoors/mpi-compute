#include <inttypes.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[]) {
  int32_t num_t = strtol(argv[1], NULL, 20);
  printf("proc cnt %" PRId32 ", \n", omp_get_num_procs());
  printf("proc cnt %" PRId32 ", \n", omp_get_num_threads());
#pragma omp parallel num_threads(num_t)
  {
    printf("Thread %" PRId32 " is running !\n", omp_get_thread_num());
    printf("cnt %" PRId32 "\n", omp_get_num_threads());
  }
  printf("cnt %" PRId32 "\n", omp_get_num_threads());
  return 0;
}