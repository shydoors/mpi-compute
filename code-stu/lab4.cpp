#include <fstream>
#include <mpi.h>
#include <nlohmann/json.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string>
using json = nlohmann::json;
int main(int argc, char *argv[]) {
  //  1. MPI环境初始化
  int rank, comm_size; // rank: 当前进程编号 | comm_size: 总进程数
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  // 矩阵维度与全局/局部矩阵指针
  int m = 0, k = 0, n = 0;          // A(m×k), B(k×n), C(m×n)
  float *A = nullptr, *B = nullptr; // 全局矩阵(仅根进程)
  float *C = nullptr;               // 结果矩阵(仅根进程)
  float *local_A = nullptr;         // 进程本地A分块
  float *local_C = nullptr;         // 进程本地C结果
  // MPI派生数据类型：用于按行/按矩阵传输数据
  MPI_Datatype RowType_A, RowType_C, MatrixType_B;
  //  2. 根进程：读取JSON矩阵
  if (0 == rank) {
    printf("========== MPI矩阵乘法并行计算 ==========\n");
    printf("根进程(rank=0)开始读取mat.json矩阵文件\n");
    // 打开并解析JSON文件
    std::ifstream file("mat.json");
    if (!file.is_open()) {
      fprintf(stderr, "错误：无法打开mat.json文件！\n");
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    json j;
    file >> j;
    file.close();
    // 从JSON解析矩阵A、B
    std::vector<std::vector<double>> mat_A = j["mat_A"];
    std::vector<std::vector<double>> mat_B = j["mat_B"];
    // 自动获取矩阵维度
    m = mat_A.size();    // A的行数
    k = mat_A[0].size(); // A的列数
    n = mat_B[0].size(); // B的列数

    // 校验矩阵乘法合法性：A的列数 == B的行数
    if (mat_B.size() != k) {
      fprintf(stderr, "错误：矩阵维度不匹配！A列数=%d，B行数=%zu\n", k,
              mat_B.size());
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    // 校验进程数可整除A的行数（行划分要求）
    if (m % comm_size != 0) {
      fprintf(stderr, "错误：A的行数(%d)不能被进程数(%d)整除！\n", m,
              comm_size);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // 分配全局矩阵内存（行优先连续存储）
    A = (float *)malloc(m * k * sizeof(float));
    B = (float *)malloc(k * n * sizeof(float));
    C = (float *)malloc(m * n * sizeof(float));
    if (!A || !B || !C) {
      fprintf(stderr, "错误：全局矩阵内存分配失败！\n");
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // JSON二维数组 -> 一维连续内存
    for (int i = 0; i < m; i++) {
      for (int j = 0; j < k; j++) {
        A[i * k + j] = (float)mat_A[i][j];
      }
    }
    for (int i = 0; i < k; i++) {
      for (int j = 0; j < n; j++) {
        B[i * n + j] = (float)mat_B[i][j];
      }
    }
    printf("矩阵维度：A(%d×%d)  B(%d×%d)  并行进程数：%d\n\n", m, k, k, n,
           comm_size);
  }
  MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&k, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Type_contiguous(k, MPI_FLOAT, &RowType_A);
  MPI_Type_commit(&RowType_A);
  MPI_Type_contiguous(n, MPI_FLOAT, &RowType_C);
  MPI_Type_commit(&RowType_C);
  MPI_Type_contiguous(k * n, MPI_FLOAT, &MatrixType_B);
  MPI_Type_commit(&MatrixType_B);
  int local_rows = m / comm_size;
  local_A = (float *)malloc(local_rows * k * sizeof(float));
  local_C = (float *)malloc(local_rows * n * sizeof(float));

  if (rank != 0) {
    B = (float *)malloc(k * n * sizeof(float));
  }
  if (!local_A || !local_C || (!B && rank != 0)) {
    fprintf(stderr, "进程%d：本地内存分配失败！\n", rank);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  MPI_Scatter(A, local_rows, RowType_A,       // 发送：根进程发local_rows行A
              local_A, local_rows, RowType_A, // 接收：每个进程存本地A分块
              0, MPI_COMM_WORLD);
  MPI_Bcast(B, 1, MatrixType_B, 0, MPI_COMM_WORLD);
  for (int i = 0; i < local_rows; i++) {
    for (int j = 0; j < n; j++) {
      local_C[i * n + j] = 0.0f;
      for (int l = 0; l < k; l++) {
        local_C[i * n + j] += local_A[i * k + l] * B[l * n + j];
      }
    }
  }
  printf("进程%d：本地矩阵计算完成\n", rank);
  MPI_Gather(local_C, local_rows, RowType_C, // 发送：每个进程的本地C
             C, local_rows, RowType_C,       // 接收：根进程拼接完整C
             0, MPI_COMM_WORLD);
  if (rank == 0) {
    printf("\n========== 计算结果矩阵C ==========\n");
    for (int i = 0; i < m; i++) {
      for (int j = 0; j < n; j++) {
        printf("%.1f ", C[i * n + j]);
      }
      printf("\n");
    }
    json j;
    std::ifstream read_file("mat.json");
    read_file >> j;
    read_file.close();
    std::vector<std::vector<double>> res(m, std::vector<double>(n));
    for (int i = 0; i < m; i++) {
      for (int j = 0; j < n; j++) {
        res[i][j] = (double)C[i * n + j];
      }
    }
    j["result"] = res;
    std::ofstream write_file("mat.json");
    write_file << j.dump(4); // 格式化缩进4空格
    write_file.close();
    printf("\n结果已成功回写入 mat.json！\n");
  }
  MPI_Type_free(&RowType_A);
  MPI_Type_free(&RowType_C);
  MPI_Type_free(&MatrixType_B);
  free(local_A);
  free(local_C);
  if (rank == 0) {
    free(A);
    free(B);
    free(C);
  } else {
    free(B);
  }
  MPI_Finalize();
  return 0;
}