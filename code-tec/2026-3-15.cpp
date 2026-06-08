#include <iostream>
#include "mpi.h"

using namespace std;

int main(int argc, char * argv[]) 
{

	int rank, size;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//任意一个CPU 向别的CPU发送的数据长度都一样。
	//这个例子中，我们使用的数据长度是1.
	//你可以改成2 看看有什么不同。
	int sendcount = 1;
	//recvcount 和 sendcount 必须相等
	int recvcount = sendcount;

	//发送缓冲区和接收缓冲区的数据长度
	int len = sendcount * size;
	int *sendbuf = new int[len];
	int *recvbuf = new int[len];

	//给发送缓冲区赋值
	for (int i = 0; i < len; i++) sendbuf[i] = i + size * rank;

	MPI_Alltoall(sendbuf, sendcount, MPI_INT, recvbuf, recvcount, MPI_INT, MPI_COMM_WORLD);

	//输出发送缓冲区的值
	for (int i = 0; i < size; i++) {
		if (i == rank) {
			cout << "Sendbuf on process " << rank << " holds " << len <<
				" data :" << endl;
			for (int j = 0; j < len; j++)
				cout << sendbuf[j] << " ";
			cout << endl;
		}
		MPI_Barrier(MPI_COMM_WORLD);
	}

	if (rank == 0) cout << endl;
	MPI_Barrier(MPI_COMM_WORLD);
	//输出接收缓冲区的值
	for (int i = 0; i < size; i++) {
		if (i == rank) {
			cout << "Recvbuf on process " << rank << " holds " << len <<
				" data :" << endl;
			for (int j = 0; j < len; j++)
				cout << recvbuf[j] << " ";
			cout << endl;
		}
		MPI_Barrier(MPI_COMM_WORLD);
	}

	delete[] sendbuf;
	delete[] recvbuf;

	MPI_Finalize();
	return EXIT_SUCCESS;

}