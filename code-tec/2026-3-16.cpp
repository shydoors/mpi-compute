//打包和解包
#include <stdio.h>
#include "mpi.h"

int main(int argc, char **argv) 
{
	int rank;
	int packsize, position;
	int a;
	double b;
	char packbuf[100];

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	do {
		if (rank == 0) 
		{
			scanf("%d %lf", &a, &b);
			packsize = 0;  /*打包开始位置*/
			MPI_Pack(&a, 1, MPI_INT, packbuf, 100, &packsize, MPI_COMM_WORLD); /*将整数a打包*/
			MPI_Pack(&b, 1, MPI_DOUBLE, packbuf, 100, &packsize, MPI_COMM_WORLD); /*将双精度数b打包*/
		}

		MPI_Bcast(&packsize, 1, MPI_INT, 0, MPI_COMM_WORLD); /*广播打包数据的大小*/
		MPI_Bcast(packbuf, packsize, MPI_PACKED, 0, MPI_COMM_WORLD); /*广播打包的数据*/

		if (rank != 0) 
		{
			position = 0;
			MPI_Unpack(packbuf, packsize, &position, &a, 1, MPI_INT, MPI_COMM_WORLD); /*其他进程先将a解包*/
			MPI_Unpack(packbuf, packsize, &position, &b, 1, MPI_DOUBLE, MPI_COMM_WORLD); /*再将b解包*/
		}

		printf("Process %d got %d and %lf\n", rank, a, b);
	} while (a >= 0); /*若a为负数则结束，否则继续上述过程*/

	MPI_Finalize();

	return 0;
}
