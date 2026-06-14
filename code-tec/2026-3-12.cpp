#include <iostream>
#include <mpi.h>
#include <vector>
int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  const int arraySize = 8;
  std::vector<int> globalData(arraySize);
  if (rank == 0) {
    // 在主进程中初始化数据
    for (int i = 0; i < arraySize; ++i) {
      globalData[i] = i * i;
    }
  }
  // 每个进程创建一个接收缓冲区，用于存储其分配到的数据
  std::vector<int> localData(arraySize / size);
  MPI_Scatter(globalData.data(), arraySize / size, MPI_INT, localData.data(),
              arraySize / size, MPI_INT, 0, MPI_COMM_WORLD);
  // 每个进程打印其接收到的数据
  std::cout << "Process " << rank << ": Received data: ";
  for (int i = 0; i < localData.size(); ++i) {
    std::cout << localData[i] << " ";
  }
  std::cout << std::endl;
  MPI_Finalize();
  return 0;
}