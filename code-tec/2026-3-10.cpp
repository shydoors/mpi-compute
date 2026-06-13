// 求方差
#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
/**创建包含随机数的数组，每个数值范围在 0 ~ 1 之间*/
float *create_rand_nums(int num_elements) {
  float *rand_nums = (float *)malloc(sizeof(float) * num_elements);
  assert(rand_nums != NULL);
  int i;
  for (i = 0; i < num_elements; i++) {
    rand_nums[i] = (rand() / (float)RAND_MAX);
  }
  return rand_nums;
}
int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: avg num_elements_per_proc\n");
    exit(1);
  }
  int num_elements_per_proc = atoi(argv[1]);
  MPI_Init(NULL, NULL);
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  /** 在所有进程上创建随机数数组*/
  srand(time(NULL) * world_rank);
  /** 为每个进程设置唯一的随机数种子*/
  float *rand_nums = NULL;
  rand_nums = create_rand_nums(num_elements_per_proc);
  /**
   * 本地计算元素和
   */
  float local_sum = 0;
  int i;
  for (i = 0; i < num_elements_per_proc; i++) {
    local_sum += rand_nums[i];
  }
  // 将所有进程的本地和归约到全局和，用于计算全局均值
  float global_sum;
  MPI_Allreduce(&local_sum, &global_sum, 1, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
  float mean = global_sum / (num_elements_per_proc * world_size);
  // 计算本地平方差和：每个元素与全局均值之差的平方和
  float local_sq_diff = 0;
  for (i = 0; i < num_elements_per_proc; i++) {
    local_sq_diff += (rand_nums[i] - mean) * (rand_nums[i] - mean);
  }
  // 将所有进程的平方差和归约到根进程（rank 0）
  float global_sq_diff;
  MPI_Reduce(&local_sq_diff, &global_sq_diff, 1, MPI_FLOAT, MPI_SUM, 0,
             MPI_COMM_WORLD);
  // 标准差 = 平方差均值开根号
  // 方差 = 平方差均值
  if (world_rank == 0) {
    float variance =
        global_sq_diff / (num_elements_per_proc * world_size); // 方差
    float stddev = sqrt(variance);                             // 标准差
    printf("均值 = %f, 方差 = %f, 标准差 = %f\n", mean, variance, stddev);
  }
  // 释放内存
  free(rand_nums);
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
  return 0;
}