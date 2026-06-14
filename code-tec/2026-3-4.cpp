#include "mpi.h"
#include <iostream>
using namespace std;
int main(int argc, char *argv[]) {
  int rank, value, size;
  MPI_Status status;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  /* 得到当前进程标识和总的进程个数*/
  do {
    /* 循环执行直到输入的数据为负时才退出*/
    if (rank == 0) {
      cout << "Please give new value=" << endl;
      cin >> value;
      cout << rank << "read <-<-" << value << endl;
      if (size > 1) {
        MPI_Send(&value, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
        cout << rank << "send " << value << "<-<-" << rank + 1 << endl;
      }
    } else {
      MPI_Recv(&value, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD,
               &status); /* 其它进程从前一个进程接收传递过来的数据*/
      cout << rank << "receive " << value << "<-<-" << rank - 1 << endl;
      if (rank < size - 1) {
        MPI_Send(&value, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
        cout << rank << "send " << value << "<-<-" << rank + 1 << endl;
      }
    }
    MPI_Barrier(
        MPI_COMM_WORLD); /* 执行一下同步加入它主要是为了将前后两次数据传递分开*/
  } while (value >= 0);
  MPI_Finalize();
  return 0;
}