#include "mpi.h"
#include <stdlib.h>
#include <iostream>
#include <iomanip>
using namespace std;
int main(int argc, char * argv[])
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int rank, size, i, num[100];
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(comm, &rank);
	MPI_Comm_size(comm, &size);
	MPI_Gather(&rank, 1, MPI_INT, num, 1, MPI_INT, 0, comm);
	/* 进程0从通信域中的所有进程收集数据并存储在数组num中 */
	if (rank == 0)
	{
		cout << "Process 0 gather from other Process:" << endl;
		for (i = 0; i < size; i++)
		{
			cout << setw(4) << num[i];
			if ((i + 1) % 4 == 0)
				cout << endl;
		}
		cout << endl;
	}
	MPI_Finalize();
	return 0;
}