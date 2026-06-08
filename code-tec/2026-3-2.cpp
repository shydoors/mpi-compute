//0号进程与1号进程间相互通信
#include <iostream>
#include  "mpi.h"
using namespace std;

int main(int argc, char *argv[])
{
	char message[20];
	int myrank;
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	if (myrank == 0)
	{
		strcpy_s(message, "Hello, process 1");
		MPI_Send(message, strlen(message), MPI_CHAR, 1, 99, MPI_COMM_WORLD);
		cout << "我是" << myrank << "进程" << endl;
	}
	else if (myrank == 1)
	{
		MPI_Recv(message, 20, MPI_CHAR, 0, 99, MPI_COMM_WORLD, &status);
		cout << "received :" << message << endl;
		cout << "我是" << myrank << "进程" << endl;
	}
	MPI_Finalize();
	return 0;
}
