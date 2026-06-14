

/**

 * @file 2026-3-9.cpp
 * @brief MPI 随机数求平均值
 */
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <mpi.h>

/**
 * @brief 创建一个随机数数组，每个数的取值范围为0~1
 */
float *create_rand_nums(int num_elements) {
  float *rand_nums = (float *)malloc(sizeof(float) * num_elements);
  assert(rand_nums != NULL);

  for (int i = 0; i < num_elements; i++) {
    rand_nums[i] = (rand() / (float)RAND_MAX);
  }
  return rand_nums;
}

int main(int argc, char **argv) {
  if (argc != 2) {

    std::cout << "用法: avg num_elements_per_proc" << std::endl;
    exit(1);
  }
  int num_elements_per_proc = atoi(argv[1]);

  MPI_Init(NULL, NULL);
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  srand(time(NULL) * world_rank);
  float *rand_nums = create_rand_nums(num_elements_per_proc);
  float local_sum = 0;

  for (int i = 0; i < num_elements_per_proc; i++) {
    local_sum += rand_nums[i];
  }

  std::cout << "进程-" << world_rank << " 的本地求和 = " << local_sum
            << "      平均值 = " << local_sum / num_elements_per_proc
            << std::endl;
  float global_sum;
  MPI_Reduce(&local_sum, &global_sum, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);

  if (world_rank == 0) {

    std::cout << "总求和 = " << global_sum << "   平均值 = "
              << global_sum / (world_size * num_elements_per_proc) << std::endl;
  }

  free(rand_nums);
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();

  return 0;
}