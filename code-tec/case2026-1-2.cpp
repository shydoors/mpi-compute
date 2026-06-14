//-----------  2  点对点并行---------/////
//////////////////////////////////
#include <chrono>
#include <iostream>
#include <mpi.h>
#include <random>
#include <vector>
using namespace std;
using namespace std::chrono;

// 函数：生成随机矩阵
vector<vector<int>> generateRandomMatrix(int rows, int cols, int minVal,
                                         int maxVal) {
  random_device rd;
  mt19937 gen(rd());
  uniform_int_distribution<> distrib(minVal, maxVal);

  vector<vector<int>> matrix(rows, vector<int>(cols));
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
      matrix[i][j] = distrib(gen);
    }
  }
  return matrix;
}

// 函数：打印矩阵
void printMatrix(const vector<vector<int>> &mat, const string &label) {
  cout << "\n=== " << label << " ===" << endl;
  for (const auto &row : mat) {
    for (int val : row) {
      cout << val << "\t";
    }
    cout << endl;
  }
  cout << endl;
}

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int rows, cols;

  // ========== 1. 参数分发==========
  if (rank == 0) {
    rows = atoi(argv[1]);
    cols = atoi(argv[2]);
    for (int i = 1; i < size; ++i) {
      MPI_Send(&rows, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      MPI_Send(&cols, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }
  } else {
    MPI_Recv(&rows, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&cols, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }

  // ========== 2. 计算负载均衡 ==========
  int base_rows = rows / size;
  int remainder = rows % size;
  int my_rows = base_rows + (rank == size - 1 ? remainder : 0);

  // ========== 3. 主进程生成数据并展平 ==========
  vector<vector<int>> mat1, mat2; // 仅用于小矩阵打印
  vector<int> flatMat1, flatMat2;

  if (rank == 0) {
    mat1 = generateRandomMatrix(rows, cols, 0, 100);
    mat2 = generateRandomMatrix(rows, cols, 0, 100);

    flatMat1.resize(rows * cols);
    flatMat2.resize(rows * cols);
    for (int i = 0; i < rows; ++i) {
      for (int j = 0; j < cols; ++j) {
        flatMat1[i * cols + j] = mat1[i][j];
        flatMat2[i * cols + j] = mat2[i][j];
      }
    }

    // 条件打印：输入矩阵（仅当 rows < 6）
    if (rows < 6) {
      printMatrix(mat1, "Input Matrix A");
      printMatrix(mat2, "Input Matrix B");
    }
  }

  // ========== 4. 分配局部缓冲区 ==========
  vector<int> localMat1(my_rows * cols);
  vector<int> localMat2(my_rows * cols);
  vector<int> localResult(my_rows * cols);

  // ========== 5. 数据分发==========
  if (rank == 0) {
    int my_offset = 0;
    for (int i = 0; i < my_rows * cols; ++i) {
      localMat1[i] = flatMat1[my_offset + i];
      localMat2[i] = flatMat2[my_offset + i];
    }

    int offset = base_rows * cols;
    for (int p = 1; p < size; ++p) {
      int proc_rows = base_rows + (p == size - 1 ? remainder : 0);
      int count = proc_rows * cols;

      MPI_Send(flatMat1.data() + offset, count, MPI_INT, p, 1, MPI_COMM_WORLD);
      MPI_Send(flatMat2.data() + offset, count, MPI_INT, p, 1, MPI_COMM_WORLD);
      offset += count;
    }
  } else {
    MPI_Recv(localMat1.data(), my_rows * cols, MPI_INT, 0, 1, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
    MPI_Recv(localMat2.data(), my_rows * cols, MPI_INT, 0, 1, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
  }

  // ========== 6. 局部计算（计时）==========
  auto start = high_resolution_clock::now();

  for (int i = 0; i < my_rows * cols; ++i) {
    localResult[i] = localMat1[i] + localMat2[i];
  }

  auto stop = high_resolution_clock::now();

  // ========== 7. 结果收集（替代 MPI_Gather）==========
  if (rank == 0) {
    vector<int> globalResult(rows * cols);

    // 复制自己的结果
    for (int i = 0; i < my_rows * cols; ++i) {
      globalResult[i] = localResult[i];
    }

    // 接收其他进程的结果
    int offset = base_rows * cols;
    for (int p = 1; p < size; ++p) {
      int proc_rows = base_rows + (p == size - 1 ? remainder : 0);
      int count = proc_rows * cols;

      MPI_Recv(globalResult.data() + offset, count, MPI_INT, p, 2,
               MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      offset += count;
    }

    // 条件打印：结果矩阵（仅当 rows < 6）
    if (rows < 6) {
      vector<vector<int>> resultMatrix(rows, vector<int>(cols));
      for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
          resultMatrix[i][j] = globalResult[i * cols + j];
        }
      }
      printMatrix(resultMatrix, "Result Matrix (A + B)");
    }

    // 输出耗时（始终打印）
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Time taken by matrix addition: " << duration.count()
         << " microseconds" << endl;

  } else {
    // 非主进程发送结果回主进程
    MPI_Send(localResult.data(), my_rows * cols, MPI_INT, 0, 2, MPI_COMM_WORLD);
  }

  MPI_Finalize();
  return 0;
}