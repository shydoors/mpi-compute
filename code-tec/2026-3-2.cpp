/**
 * @file 2026-3-2.cpp
 * @brief 0号进程与1号进程间相互通信
 */
#include "mpi.h"
#include <cstring>
#include <iostream>
int main(int argc, char *argv[]) {
  char message[20];
  int myrank;
  MPI_Status status;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  if (myrank == 0) {
    strcpy(message, "Hello, process 1");
    MPI_Send(message, strlen(message), MPI_CHAR, 1, 99, MPI_COMM_WORLD);
    std::cout << "我是" << myrank << "进程" << std::endl;
  } else if (myrank == 1) {
    MPI_Recv(message, 20, MPI_CHAR, 0, 99, MPI_COMM_WORLD, &status);
    std::cout << "received :" << message << std::endl;
    std::cout << "我是" << myrank << "进程" << std::endl;
  }
  MPI_Finalize();
  return 0;
}
