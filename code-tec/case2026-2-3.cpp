//-----------    集合通信---------/////
 //////////////////////////////////
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <mpi.h>
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
void printMatrix(const vector<vector<int>>& mat, const string& label) {
    cout << "\n=== " << label << " ===" << endl;
    for (const auto& row : mat) {
        for (int val : row) {
            cout << val << "\t";
        }
        cout << endl;
    }
    cout << endl;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int rows, cols;

    // ========== 1. 参数广播 ==========
    if (rank == 0) {
        rows = atoi(argv[1]);
        cols = atoi(argv[2]);
    }
    MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&cols, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int n = rows;  // 方阵维度

    // ========== 2. 负载均衡 ==========
    int base_rows = n / size;
    int remainder = n % size;
    int my_rows = base_rows + (rank == size - 1 ? remainder : 0);

    // ========== 3. 主进程生成数据 ==========
    vector<vector<int>> mat1, mat2;
    vector<int> flatA, flatB;  //  所有进程都需要声明

    if (rank == 0) {
        mat1 = generateRandomMatrix(n, n, 0, 100);
        mat2 = generateRandomMatrix(n, n, 0, 100);

        flatA.resize(n * n);
        flatB.resize(n * n);  // 主进程填充数据
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                flatA[i * n + j] = mat1[i][j];
                flatB[i * n + j] = mat2[i][j];
            }
        }

        if (n < 6) {
            printMatrix(mat1, "Matrix A");
            printMatrix(mat2, "Matrix B");
        }
    }

    // ========== 4. 分配局部缓冲区 ==========
    vector<int> localA(my_rows * n);
    vector<int> fullB(n * n);          //  所有进程都需要 B 的完整副本
    vector<int> localC(my_rows * n);

    // 关键修复：所有进程都必须为 flatB 分配缓冲区（即使 rank!=0 不填充数据）
    if (rank != 0) {
        flatB.resize(n * n);  // 非root进程也要分配接收缓冲区
    }

    // ========== 5. 数据分发（集合通信）==========

    // 5.1 分发矩阵 A：MPI_Scatterv
    if (rank == 0) {
        vector<int> sendcounts(size), displs(size);
        int offset = 0;
        for (int p = 0; p < size; ++p) {
            int proc_rows = base_rows + (p == size - 1 ? remainder : 0);
            sendcounts[p] = proc_rows * n;
            displs[p] = offset;
            offset += sendcounts[p];
        }
        MPI_Scatterv(flatA.data(), sendcounts.data(), displs.data(), MPI_INT,
            localA.data(), my_rows * n, MPI_INT, 0, MPI_COMM_WORLD);
    }
    else {
        MPI_Scatterv(nullptr, nullptr, nullptr, MPI_INT,
            localA.data(), my_rows * n, MPI_INT, 0, MPI_COMM_WORLD);
    }

    // 5.2 广播矩阵 B：MPI_Bcast ⭐ 所有进程 flatB 都已有有效指针
    MPI_Bcast(flatB.data(), n * n, MPI_INT, 0, MPI_COMM_WORLD);

    // 所有进程将 flatB 复制到 fullB（便于后续计算）
    fullB = flatB;

    // ========== 6. 局部矩阵乘法计算（计时）==========
    auto start = high_resolution_clock::now();

    for (int i = 0; i < my_rows; ++i) {
        for (int j = 0; j < n; ++j) {
            long long sum = 0;
            for (int k = 0; k < n; ++k) {
                sum += static_cast<long long>(localA[i * n + k]) * fullB[k * n + j];
            }
            localC[i * n + j] = static_cast<int>(sum);
        }
    }

    auto stop = high_resolution_clock::now();

    // ========== 7. 结果收集：MPI_Gatherv ==========
    if (rank == 0) {
        vector<int> globalC(n * n);

        vector<int> recvcounts(size), displs(size);
        int offset = 0;
        for (int p = 0; p < size; ++p) {
            int proc_rows = base_rows + (p == size - 1 ? remainder : 0);
            recvcounts[p] = proc_rows * n;
            displs[p] = offset;
            offset += recvcounts[p];
        }

        MPI_Gatherv(localC.data(), my_rows * n, MPI_INT,
            globalC.data(), recvcounts.data(), displs.data(), MPI_INT,
            0, MPI_COMM_WORLD);

        if (n < 6) {
            vector<vector<int>> resultMatrix(n, vector<int>(n));
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    resultMatrix[i][j] = globalC[i * n + j];
                }
            }
            printMatrix(resultMatrix, "Result Matrix (A × B)");
        }

        auto duration = duration_cast<microseconds>(stop - start);
        cout << "Time taken by matrix multiplication: " << duration.count() << " microseconds" << endl;

    }
    else {
        MPI_Gatherv(localC.data(), my_rows * n, MPI_INT,
            nullptr, nullptr, nullptr, MPI_INT,
            0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}