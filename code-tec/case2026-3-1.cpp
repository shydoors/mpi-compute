#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;

// 向量点积
double dot(const vector<double> &a, const vector<double> &b) {
  double sum = 0.0;
  for (size_t i = 0; i < a.size(); ++i) {
    sum += a[i] * b[i];
  }
  return sum;
}

// 向量减法：a - b
vector<double> subtract(const vector<double> &a, const vector<double> &b) {
  vector<double> res(a.size());
  for (size_t i = 0; i < a.size(); ++i) {
    res[i] = a[i] - b[i];
  }
  return res;
}

// 向量加法：a + b
vector<double> add(const vector<double> &a, const vector<double> &b) {
  vector<double> res(a.size());
  for (size_t i = 0; i < a.size(); ++i) {
    res[i] = a[i] + b[i];
  }
  return res;
}

// 标量乘向量
vector<double> scale(double s, const vector<double> &v) {
  vector<double> res(v.size());
  for (size_t i = 0; i < v.size(); ++i) {
    res[i] = s * v[i];
  }
  return res;
}

// 矩阵乘向量
vector<double> matVec(const vector<vector<double>> &A,
                      const vector<double> &x) {
  int n = A.size();
  vector<double> res(n, 0.0);
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      res[i] += A[i][j] * x[j];
    }
  }
  return res;
}

// 向量2-范数
double norm(const vector<double> &v) { return sqrt(dot(v, v)); }

// 共轭梯度法求解 Ax = b
vector<double> conjugateGradient(const vector<vector<double>> &A,
                                 const vector<double> &b, double tol = 1e-6,
                                 int maxIter = 1000) {
  int n = b.size();
  vector<double> x(n, 0.0); // 初始解
  vector<double> r = b;     // 初始残差 r = b - A*x = b
  vector<double> p = r;     // 初始搜索方向
  double rsold = dot(r, r);

  cout << "迭代开始..." << endl;

  for (int k = 0; k < maxIter; ++k) {
    vector<double> Ap = matVec(A, p);
    double pAp = dot(p, Ap);
    if (fabs(pAp) < 1e-12) {
      cerr << "错误：除零风险，p^T A p 接近零" << endl;
      break;
    }

    double alpha = rsold / pAp;
    x = add(x, scale(alpha, p));
    r = subtract(r, scale(alpha, Ap));

    double rsnew = dot(r, r);
    cout << "迭代 " << k + 1 << ": 残差范数 = " << fixed << setprecision(10)
         << sqrt(rsnew) << endl;
    if (sqrt(rsnew) < tol) {
      cout << "在 " << k + 1 << " 次迭代后收敛。" << endl;
      break;
    }

    double beta = rsnew / rsold;
    p = add(r, scale(beta, p));
    rsold = rsnew;
  }

  return x;
}

// 打印向量
void printVector(const string &label, const vector<double> &v) {
  cout << label << ": [";
  for (size_t i = 0; i < v.size(); ++i) {
    cout << fixed << setprecision(6) << v[i];
    if (i < v.size() - 1)
      cout << ", ";
  }
  cout << "]" << endl;
}

// 打印矩阵
void printMatrix(const string &label, const vector<vector<double>> &A) {
  cout << label << ":" << endl;
  for (const auto &row : A) {
    cout << "  [";
    for (size_t i = 0; i < row.size(); ++i) {
      cout << fixed << setprecision(6) << row[i];
      if (i < row.size() - 1)
        cout << ", ";
    }
    cout << "]" << endl;
  }
}

// 验证解：计算残差 ||Ax - b||
double verifySolution(const vector<vector<double>> &A, const vector<double> &x,
                      const vector<double> &b) {
  vector<double> Ax = matVec(A, x);
  vector<double> r = subtract(Ax, b);
  return norm(r);
}

int main() {
  // 示例：对称正定矩阵
  vector<vector<double>> A = {
      {4.0, 1.0, 0.0}, {1.0, 3.0, 1.0}, {0.0, 1.0, 2.0}};
  vector<double> b = {1.0, 2.0, 3.0};

  cout << "=== 共轭梯度法求解线性方程组 ===" << endl;
  printMatrix("系数矩阵 A", A);
  printVector("右端向量 b", b);
  cout << endl;

  vector<double> x = conjugateGradient(A, b);

  cout << endl;
  printVector("计算解 x", x);

  double residual = verifySolution(A, x, b);
  cout << "残差范数 ||Ax - b|| = " << fixed << setprecision(10) << residual
       << endl;

  return 0;
}