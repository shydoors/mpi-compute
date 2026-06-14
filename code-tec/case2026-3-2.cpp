#include <cmath>
#include <iomanip>
#include <iostream>
#include <mpi.h>
#include <vector>

using namespace std;

// 全局 MPI 变量
int rank, size;

// 本地向量点积
double local_dot(const vector<double> &a, const vector<double> &b) {
  double sum = 0.0;
  for (size_t i = 0; i < a.size(); ++i) {
    sum += a[i] * b[i];
  }
  return sum;
}

// 本地矩阵乘向量 (A_local 是 local_rows x N, p 是 N)
vector<double> local_matVec(const vector<vector<double>> &A_local,
                            const vector<double> &p_global) {
  int local_rows = A_local.size();
  int N = p_global.size();
  vector<double> res(local_rows, 0.0);
  for (int i = 0; i < local_rows; ++i) {
    double sum = 0.0;
    for (int j = 0; j < N; ++j) {
      sum += A_local[i][j] * p_global[j];
    }
    res[i] = sum;
  }
  return res;
}

// 主函数
int main(int argc, char **argv) {
  int rank, size;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  // ================= 1. 初始化数据 (仅 Rank 0) =================
  int N = 0;
  vector<vector<double>> A_full;
  vector<double> b_full;
  if (rank == 0) {
    N = 100; // 矩阵维度
    // 生成对称正定矩阵 (对角占优)
    A_full.resize(N, vector<double>(N, 0.0));
    b_full.resize(N);
    for (int i = 0; i < N; ++i) {
      for (int j = 0; j < N; ++j) {
        if (i == j) {
          A_full[i][j] = 10.0 + i;
        } else {
          A_full[i][j] = 1.0;
        }
      }
      b_full[i] = 1.0;
    }
    cout << "=== MPI 并行共轭梯度法 ===" << endl;
    cout << "矩阵维度 N = " << N << ", 进程数 P = " << size << endl;
  }

  // ================= 2. 广播维度 =================
  MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

  // ================= 3. 计算本地行数 (处理不能整除的情况) =================
  int base_rows = N / size;
  int remainder = N % size;
  int local_rows = base_rows + (rank < remainder ? 1 : 0);
  // 计算每个进程的行数计数 (counts) 和位移 (displs) 用于 Scatterv/Gatherv
  vector<int> counts(size);
  vector<int> displs(size);
  int current_disp = 0;
  for (int i = 0; i < size; ++i) {
    counts[i] = base_rows + (i < remainder ? 1 : 0);
    displs[i] = current_disp;
    current_disp += counts[i];
  }

  // ================= 4. 分发矩阵 A 和向量 b =================
  // 本地存储
  vector<vector<double>> A_local(local_rows, vector<double>(N));
  vector<double> b_local(local_rows);
  vector<double> x_local(local_rows, 0.0); // 解向量初始为 0
  vector<double> r_local(local_rows);
  vector<double> p_local(local_rows);
  vector<double> Ap_local(local_rows);

  // 展平矩阵以便 Scatter (MPI 处理一维数组更方便)
  vector<double> A_flat_full;
  if (rank == 0) {
    A_flat_full.reserve(N * N);
    for (const auto &row : A_full) {
      A_flat_full.insert(A_flat_full.end(), row.begin(), row.end());
    }
  }

  vector<double> A_flat_local(local_rows * N);
  vector<double> b_flat_local(local_rows); // 实际上就是 b_local

  // 使用 Scatterv 分发数据
  MPI_Scatterv(rank == 0 ? A_flat_full.data() : nullptr, counts.data(),
               displs.data(), MPI_DOUBLE, A_flat_local.data(), counts[rank] * N,
               MPI_DOUBLE, 0, MPI_COMM_WORLD);

  MPI_Scatterv(rank == 0 ? b_full.data() : nullptr, counts.data(),
               displs.data(), MPI_DOUBLE, b_local.data(), counts[rank],
               MPI_DOUBLE, 0, MPI_COMM_WORLD);

  // 恢复本地矩阵结构
  for (int i = 0; i < local_rows; ++i) {
    for (int j = 0; j < N; ++j) {
      A_local[i][j] = A_flat_local[i * N + j];
    }
  }
  // ================= 5. 共轭梯度迭代 =================
  // 初始化
  r_local = b_local; // 因为 x=0, r = b - Ax = b
  p_local = r_local;

  double rsold = local_dot(r_local, r_local);
  double rsold_global = 0.0;
  MPI_Allreduce(&rsold, &rsold_global, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

  double tol = 1e-6;
  int maxIter = 1000;

  // 用于 Allgatherv 的缓冲区
  vector<double> p_global(N);
  vector<double> r_global(N); // 仅用于验证或调试，算法中主要用局部 r

  if (rank == 0) {
    cout << "迭代开始..." << endl;
  }
  for (int k = 0; k < maxIter; ++k) {
    // --- 步骤 1: 收集完整的 p 向量到所有进程 (关键并行通信) ---
    MPI_Allgatherv(p_local.data(), counts[rank], MPI_DOUBLE, p_global.data(),
                   counts.data(), displs.data(), MPI_DOUBLE, MPI_COMM_WORLD);
    // --- 步骤 2: 本地矩阵乘法 Ap = A * p ---
    Ap_local = local_matVec(A_local, p_global);
    // --- 步骤 3: 计算 p^T Ap (需要全局归约) ---
    double pAp_local = local_dot(p_local, Ap_local);
    double pAp_global = 0.0;
    MPI_Allreduce(&pAp_local, &pAp_global, 1, MPI_DOUBLE, MPI_SUM,
                  MPI_COMM_WORLD);

    if (fabs(pAp_global) < 1e-12) {
      if (rank == 0) {
        cerr << "错误：除零风险" << endl;
      }
      break;
    }
    // --- 步骤 4: 计算 alpha ---
    double alpha = rsold_global / pAp_global;
    // --- 步骤 5: 更新 x 和 r (本地操作) ---
    for (int i = 0; i < local_rows; ++i) {
      x_local[i] += alpha * p_local[i];
      r_local[i] -= alpha * Ap_local[i];
    }

    // --- 步骤 6: 计算新的残差范数 (需要全局归约) ---
    double rsnew_local = local_dot(r_local, r_local);
    double rsnew_global = 0.0;
    MPI_Allreduce(&rsnew_local, &rsnew_global, 1, MPI_DOUBLE, MPI_SUM,
                  MPI_COMM_WORLD);

    if (rank == 0) {
      cout << "迭代 " << k + 1 << ": 残差范数 = " << fixed << setprecision(10)
           << sqrt(rsnew_global) << endl;
    }

    if (sqrt(rsnew_global) < tol) {
      if (rank == 0) {
        cout << "在 " << k + 1 << " 次迭代后收敛。" << endl;
      }
      break;
    }

    // --- 步骤 7: 计算 beta 和更新 p (本地操作) ---
    double beta = rsnew_global / rsold_global;
    for (int i = 0; i < local_rows; ++i) {
      p_local[i] = r_local[i] + beta * p_local[i];
    }
    rsold_global = rsnew_global;
  }

  // ================= 6. 收集结果到 Rank 0 进行验证 =================
  vector<double> x_full(N);
  MPI_Gatherv(x_local.data(), counts[rank], MPI_DOUBLE, x_full.data(),
              counts.data(), displs.data(), MPI_DOUBLE, 0, MPI_COMM_WORLD);

  if (rank == 0) {
    // 验证残差 ||Ax - b||
    vector<double> Ax(N, 0.0);
    for (int i = 0; i < N; ++i) {
      for (int j = 0; j < N; ++j) {
        Ax[i] += A_full[i][j] * x_full[j];
      }
    }
    double residual_sq = 0.0;
    for (int i = 0; i < N; ++i) {
      double diff = Ax[i] - b_full[i];
      residual_sq += diff * diff;
    }
    cout << "最终残差范数 ||Ax - b|| = " << fixed << setprecision(10)
         << sqrt(residual_sq) << endl;
    cout << "解向量前 5 个元素：";
    for (int i = 0; i < min(5, N); ++i) {
      cout << x_full[i] << " ";
    }
    cout << "..." << endl;
  }

  MPI_Finalize();
  return 0;
}