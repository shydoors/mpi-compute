//广播的使用
#include <stdio.h>
#include <iostream>
#include "mpi.h"
using namespace std;
int main(int argc, char *argv[]) 
{
	int rank, nproc;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nproc);
	int data = 0;
	//主进程时把data赋值为99
	if (rank == 0) 
	{
		data = 99;
	}
	//这里就可以把data的值发出去，如果没有这个函数的话0号进程中的data就会和其他进程中的data不一样。
	MPI_Bcast(&data, 1, MPI_INT, 0, MPI_COMM_WORLD);
	cout << "data = " << data << " in " << rank << " process." << endl;
	MPI_Finalize();
	return 0;
}