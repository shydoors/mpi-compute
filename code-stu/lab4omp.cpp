#include <chrono>
#include <iostream>
#include <omp.h>
#include <vector>
/**
 * OMP版本
 */
std::vector<std::vector<int>> Mat_A = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
std::vector<std::vector<int>> Mat_B = {{9, 8, 7}, {6, 5, 4}, {3, 2, 1}};
std::vector<std::vector<int>>
Multiply_omp(const std::vector<std::vector<int>> &A,
             const std::vector<std::vector<int>> &B) {
  size_t rows = A.size();
  size_t cols = B[0].size();
  size_t inner = B.size();
  if (A.empty() || B.empty() || A[0].size() != B.size()) {
    std::cerr << "Error: Matrix dimensions do not match for multiplication."
              << std::endl;
    return {}; // 返回空矩阵
  }
  std::vector<std::vector<int>> ans(A.size(), std::vector<int>(B[0].size()));
/**
 * 多线程矩阵乘法
 * 结果矩阵的每一个元素的实现都单独用一个线程独立计算，最后返回给主线程
 * 注意，执行的是A*B,记得转置等问题
 * 规模不匹配直接报错，拒绝执行运算
 */
#pragma omp parallel for collapse(2) schedule(static)
  for (size_t i = 0; i < rows; ++i) {
    for (size_t j = 0; j < cols; ++j) {
      int sum = 0;
      // 内积累加，编译器会自动优化
      for (size_t k = 0; k < inner; ++k) {
        sum += A[i][k] * B[k][j];
      }
      ans[i][j] = sum;
    }
  }
  return ans;
};
int main() {
  auto start = std::chrono::high_resolution_clock::now();
  auto result = Multiply_omp(Mat_A, Mat_B);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  std::cout << "Time taken: " << elapsed.count() << " seconds\n";
  std::cout << "Result Matrix:\n";
  for (const auto &row : result) {
    for (int val : row) {
      std::cout << val << " ";
    }
    std::cout << "\n";
  }
  return 0;
}