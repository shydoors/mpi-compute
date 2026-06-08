//-----------  3  集合通信---------/////
 //////////////////////////////////
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <mpi.h> // MPI 头文件
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

// 函数：打印矩阵
void printMatrix(const vector<vector<int>>& mat) {
    for (const auto& row : mat) {
        for (int val : row) {
            cout << val << "\t";
        }
        cout << endl;
    }
    cout << endl;
}
int main(int argc, char** argv) {
    // 初始化 MPI
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // 获取当前进程的 rank
    MPI_Comm_size(MPI_COMM_WORLD, &size); // 获取总进程数

    int rows, cols;
    if (rank == 0) {
        rows = atoi(argv[1]);
        cols = atoi(argv[2]);
    }

    // 广播矩阵的行数和列数到所有进程
    MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&cols, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // 主进程生成全局矩阵
    vector<vector<int>> mat1, mat2;
    vector<int> flatMat1, flatMat2;
    if (rank == 0) {
        mat1 = generateRandomMatrix(rows, cols, 0, 100);
        mat2 = generateRandomMatrix(rows, cols, 0, 100);

        // 将二维矩阵展平为一维数组
        flatMat1.resize(rows * cols);
        flatMat2.resize(rows * cols);
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                flatMat1[i * cols + j] = mat1[i][j];
                flatMat2[i * cols + j] = mat2[i][j];
            }
        }

        // 【修改】如果行数 <= 5，打印生成的输入矩阵
        if (rows <= 5) {
            cout << "===== Rank 0: Input Matrices =====" << endl;
            cout << "First matrix (" << rows << "x" << cols << "):" << endl;
            printMatrix(mat1);
            cout << endl;
            cout << "Second matrix (" << rows << "x" << cols << "):" << endl;
            printMatrix(mat2);
            cout << "==================================" << endl << endl;
        }
    }

    // 计算每个进程分得的行数（基础分配）
    int base_rows = rows / size;
    int remainder = rows % size;

    // 计算当前进程实际处理的行数
    int rows_per_process = base_rows;
    if (rank == size - 1) {
        rows_per_process += remainder;  // 最后一个进程处理剩余行
    }

    // 为每个进程分配局部矩阵空间
    vector<int> localMat1(rows_per_process * cols);
    vector<int> localMat2(rows_per_process * cols);
    vector<int> localResult(rows_per_process * cols);

    // 设置 sendcounts 和 displs（用于 Scatterv/Gatherv）
    int* sendcounts = new int[size];
    int* displs = new int[size];
    int offset = 0;
    for (int i = 0; i < size; ++i) {
        int proc_rows = base_rows;
        if (i == size - 1) proc_rows += remainder;
        sendcounts[i] = proc_rows * cols;
        displs[i] = offset;
        offset += sendcounts[i];
    }

    // 计算当前进程的 recvcount 和 recvdispl（本地偏移）
    int recvcount = rows_per_process * cols;
    int recvdispl = displs[rank];  // 当前进程在一维数组中的起始位置

    // 分发矩阵（使用 Scatterv 支持非均匀分发）
    MPI_Scatterv(flatMat1.data(), sendcounts, displs, MPI_INT,
        localMat1.data(), recvcount, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatterv(flatMat2.data(), sendcounts, displs, MPI_INT,
        localMat2.data(), recvcount, MPI_INT, 0, MPI_COMM_WORLD);

    // 每个进程计算自己的局部结果
    auto start = high_resolution_clock::now(); // 开始计时
    for (int i = 0; i < recvcount; ++i) {
        localResult[i] = localMat1[i] + localMat2[i];
    }
    auto stop = high_resolution_clock::now(); // 结束计时

    // 主进程分配全局结果矩阵
    vector<int> globalResult(rows * cols);

    // 将局部结果收集到主进程（使用 Gatherv 匹配 Scatterv）
    MPI_Gatherv(localResult.data(), recvcount, MPI_INT,
        globalResult.data(), sendcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);

    // 主进程输出结果和耗时
    if (rank == 0) {
        // 将一维结果转回二维矩阵
        vector<vector<int>> resultMatrix(rows, vector<int>(cols));
        for (int i = 0; i < rows; ++i) {
            copy(globalResult.begin() + i * cols,
                globalResult.begin() + (i + 1) * cols,
                resultMatrix[i].begin());
        }

        // 【修改】如果行数 <= 5，打印结果矩阵
        if (rows <= 5) {
            cout << "===== Rank 0: Result Matrix =====" << endl;
            cout << "Resultant matrix after addition (" << rows << "x" << cols << "):" << endl;
            printMatrix(resultMatrix);
            cout << "=================================" << endl << endl;
        }

        // 输出耗时（所有情况都输出）
        auto duration = duration_cast<microseconds>(stop - start);
        cout << "[Rank 0] Time taken by matrix addition: "
            << duration.count() << " microseconds"
            << " (" << duration.count() / 1000.0 << " ms)" << endl;
    }

    // 释放动态分配的内存
    delete[] sendcounts;
    delete[] displs;

    // 结束 MPI
    MPI_Finalize();
    return 0;
}