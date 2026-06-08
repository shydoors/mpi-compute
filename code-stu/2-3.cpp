#include <iostream>
#include <omp.h>
int main() {
  int32_t sumx = 1, sumy = 2;
#pragma omp parallel num_threads(4) shared(sumx, sumy)
  {
#pragma omp atomic
    sumx += omp_get_thread_num();
#pragma omp atomic
    sumy += omp_get_thread_num();
  }
  printf(" sumx = %d ,sumy = %d \n",sumx,sumy);
  return 0;
}