////-----------     矩阵乘法---------/////
// //////////////////////////////////
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
using namespace std;
using namespace std::chrono;

// 函数：生成随机矩阵
vector<vector<int>> generateRandomMatrix(int rows, int cols, int minVal, int maxVal) {
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

// 函数：矩阵乘法 ⭐ (要求 mat1.cols == mat2.rows)
vector<vector<int>> matrixMultiplication(const vector<vector<int>>& mat1,
    const vector<vector<int>>& mat2) {
    int rows1 = mat1.size(), cols1 = mat1[0].size();
    int rows2 = mat2.size(), cols2 = mat2[0].size();

    // 维度检查
    if (cols1 != rows2) {
        cerr << "Error: Incompatible dimensions for multiplication!" << endl;
        return {};
    }

    vector<vector<int>> result(rows1, vector<int>(cols2, 0));  // 初始化为0

    // 标准三重循环：C[i][j] = Σ A[i][k] * B[k][j]
    for (int i = 0; i < rows1; ++i) {
        for (int j = 0; j < cols2; ++j) {
            for (int k = 0; k < cols1; ++k) {
                result[i][j] += mat1[i][k] * mat2[k][j];  // 👈 乘法 + 累加
            }
        }
    }
    return result;
}

// 函数：打印矩阵
void printMatrix(const vector<vector<int>>& mat, const string& label = "") {
    if (!label.empty()) cout << "\n=== " << label << " ===" << endl;
    for (const auto& row : mat) {
        for (int val : row) {
            cout << val << "\t";  // 用 tab 对齐，更美观
        }
        cout << endl;
    }
}

int main() {
    int rows, cols;

    // 输入矩阵维度
    cout << "Enter the number of rows: ";
    cin >> rows;
    cout << "Enter the number of columns: ";
    cin >> cols;

    // 生成两个随机矩阵
    const int minVal = 0, maxVal = 100;
    auto mat1 = generateRandomMatrix(rows, cols, minVal, maxVal);
    auto mat2 = generateRandomMatrix(rows, cols, minVal, maxVal);

    // 打印输入矩阵（仅当矩阵较小时，避免刷屏）
    if (rows <= 6 && cols <= 6) {
        printMatrix(mat1, "Matrix A");
        printMatrix(mat2, "Matrix B");
    }

    // 计算加法并计时
    auto start = high_resolution_clock::now();
    auto result = matrixMultiplication(mat1, mat2);
    auto stop = high_resolution_clock::now();

    // 打印结果矩阵（小矩阵时）
    if (rows <= 6 && cols <= 6) {
        printMatrix(result, "Result (A + B)");
    }

    //  输出耗时（修复单位错误）
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "\nTime taken by matrix addition: "
        << duration.count() << " microseconds" << endl;  // 直接输出微秒值

    return 0;
}