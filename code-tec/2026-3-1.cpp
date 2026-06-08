//简单的MPI程序
#include <iostream>
#include <string>
#include  "mpi.h"
using namespace std;
int main(int argc, char *argv[])
{
	MPI_Init(&argc, &argv);
	string s = argv[1];
	string s1 = argv[2];
	cout<<s <<"Hello World!" <<"   "<<s1<< endl;
	MPI_Finalize();
	return 0;
}