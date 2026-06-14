/**
 * @file 2026-3-1.cpp
 * @brief 简单的 MPI 程序
 */
#include "mpi.h"
#include <iostream>
#include <string>
int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);
  std::string s = argv[1];
  std::string s1 = argv[2];
  std::cout << s << "Hello World!   " << s1 << std::endl;
  MPI_Finalize();
  return 0;
}