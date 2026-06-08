//多个进程与0进程通信
#include <iostream>
#include  "mpi.h"
using namespace std;
int main(int argc, char *argv[])
{
	int rank, size, i, buf[1]; 
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	if (rank == 0) 
	{ 
		for (i = 0; i < 100 * (size - 1); i++) 
		{ 
			MPI_Recv(buf, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); 
			cout << "Msg=" << buf[0] << " from " << status.MPI_SOURCE << " with tag " << status.MPI_TAG << endl;
		}
	}
	else 
	{ 
		for (i = 0; i < 100; i++)
			buf[0] = rank + i; 
		MPI_Send(buf, 1, MPI_INT, 0, i, MPI_COMM_WORLD); 
	}
	MPI_Finalize();
	return 0;
}
