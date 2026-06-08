//自定义类型
#include <stdio.h>
#include <iostream>
#include "mpi.h"
using namespace std;
struct userinfo
{
	int age;
	double weight;
	char sex;
};
int main(int argc, char *argv[])
{
	int rank, nproc;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nproc);
	MPI_Aint displacement[3] =
	{
		0,
		8,
		16
	};
	int types[3] = { MPI_INT,MPI_DOUBLE,MPI_CHAR };
	int blockLength[3] = { 1,1,1};
	MPI_Datatype newtype;
	MPI_Type_create_struct(3, blockLength, displacement, types, &newtype);
	MPI_Type_commit(&newtype);

	userinfo  ui;
	//int data = 0;
	//主进程时把data赋值为99
	if (rank == 0)
	{
		ui.age = 30;
		ui.weight = 154.34;
		ui.sex = 'm';
	}
	//这里就可以把data的值发出去，如果没有这个函数的话0号进程中的data就会和其他进程中的data不一样。
	MPI_Bcast(&ui, 1, newtype, 0, MPI_COMM_WORLD);

	cout << "data  in " << rank << " process." << endl;
	cout << ui.age << endl;
	cout << ui.weight << endl;
	cout << ui.sex << endl;
	MPI_Type_free(&newtype);
	MPI_Finalize();
	return 0;
}
