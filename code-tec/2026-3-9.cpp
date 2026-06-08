#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>
#include <time.h>
using namespace std;
// 创建一个随机数数组。每个数的取值范围为0 - 1
float* create_rand_nums(int num_elements)
{
	float* rand_nums = (float*)malloc(sizeof(float) * num_elements);
	assert(rand_nums != NULL);
	int i;
	for (i = 0; i < num_elements; i++)
	{
		rand_nums[i] = (rand() / (float)RAND_MAX);
	}
	return rand_nums;
}
int main(int argc, char** argv)
{
	if (argc != 2)
	{
		cout << "用法: avg num_elements_per_proc" << endl;
		exit(1);
	}
	int num_elements_per_proc = atoi(argv[1]);
	string aaa = argv[0];
	MPI_Init(NULL, NULL);
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	// 在所有进程上创建一个随机数组
	srand(time(NULL) * world_rank);   // 初始化随机数生成器，使得每个进程得到不同的结果
	float* rand_nums = NULL;
	rand_nums = create_rand_nums(num_elements_per_proc);
	// 计算本地求和
	float local_sum = 0;
	int i;
	for (i = 0; i < num_elements_per_proc; i++)
	{
		local_sum += rand_nums[i];
	}
	// 在每个进程上打印随机数
	cout << "进程-" << world_rank << " 的本地求和 = " << local_sum << "      平均值 = " << local_sum / num_elements_per_proc << endl;
	// 将所有本地求和归约为全局求和
	float global_sum;
	MPI_Reduce(&local_sum, &global_sum, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
	// 打印结果
	if (world_rank == 0)
	{
		cout << "总求和 = " << global_sum << "   平均值 = " << global_sum / (world_size * num_elements_per_proc) << endl;
	}
	// 清理
	free(rand_nums);
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
	cout << aaa.c_str() << endl;
}