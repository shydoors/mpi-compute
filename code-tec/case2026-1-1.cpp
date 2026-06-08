//-----------  1   矩阵加法---------/////
 //////////////////////////////////
#include <iostream>
#include <vector>
#include <random>
#include <chrono> // 用于计时
using namespace std;
using namespace std::chrono; // 计时库的命名空间

// 函数：生成随机矩阵
vector<vector<int>> generateRandomMatrix(int rows, int cols, int minVal, int maxVal) {
    random_device rd;  // 随机数种子
    mt19937 gen(rd()); // 随机数引擎
    uniform_int_distribution<> distrib(minVal, maxVal); // 均匀分布

    vector<vector<int>> matrix(rows, vector<int>(cols));
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            matrix[i][j] = distrib(gen); // 生成随机数
        }
    }
    return matrix;
}

// 函数：矩阵相加
vector<vector<int>> matrixAddition(const vector<vector<int>>& mat1, const vector<vector<int>>& mat2) {
    int rows = mat1.size();
    int cols = mat1[0].size();
    vector<vector<int>> result(rows, vector<int>(cols));

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            result[i][j] = mat1[i][j] + mat2[i][j];
        }
    }

    return result;
}

// 函数：打印矩阵
void printMatrix(const vector<vector<int>>& mat) {
    for (const auto& row : mat) {
        for (int val : row) {
            cout << val << " ";
        }
        cout << endl;
    }
}

int main() {
    int rows, cols;

    // 输入矩阵的行数和列数
    cout << "Enter the number of rows: ";
    cin >> rows;
    cout << "Enter the number of columns: ";
    cin >> cols;

    // 随机生成两个矩阵
    int minVal = 0, maxVal = 100; // 随机数范围
    auto mat1 = generateRandomMatrix(rows, cols, minVal, maxVal);
    auto mat2 = generateRandomMatrix(rows, cols, minVal, maxVal);

    // 打印生成的矩阵（仅当行数 <= 4 时）
    if (rows <= 4) {
        cout << "First matrix:" << endl;
        printMatrix(mat1);
        cout << endl;

        cout << "Second matrix:" << endl;
        printMatrix(mat2);
        cout << endl;
    }
    else {
        cout << "Matrices not printed (rows > 4)" << endl;
    }

    // 计算矩阵相加并统计时间
    auto start = high_resolution_clock::now(); // 开始计时
    auto result = matrixAddition(mat1, mat2);
    auto stop = high_resolution_clock::now(); // 结束计时

    // 输出结果矩阵（仅当行数 <= 4 时）
    if (rows <= 4) {
        cout << "Resultant matrix after addition:" << endl;
        printMatrix(result);
        cout << endl;
    }

    // 计算并输出耗时
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Time taken by matrix addition: " << duration.count() << " microseconds" << endl;
    cout << "                 (or " << duration.count() / 1000.0 << " milliseconds)" << endl;

    return 0;
}