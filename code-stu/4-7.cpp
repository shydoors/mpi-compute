#include <cstdint>
#include <iostream>
#include <mpi.h>
#include <vector>
#define N 10
int main(int argc, char *argv[]) {
  int32_t rank, size;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  std::vector<int64_t> send_buf(N);
  for (int i = 0; i < N; ++i) {
    send_buf[i] = i + rank * 10;
  }
  std::vector<int64_t> recv_buf(size * N);
  MPI_Allgather(send_buf.data(), N, MPI_INT64_T, recv_buf.data(), N,
                MPI_INT64_T, MPI_COMM_WORLD);
  if (0 == rank) {
    std::cout << "=== Gathered data ===" << std::endl;
    for (auto v : recv_buf) {
      std::cout << v << std::endl;
    }
  }
  MPI_Finalize();
  return 0;
}