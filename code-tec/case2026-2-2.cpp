//-----------    点对点并行---------/////
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

    // ========== 1. 参数分发 ==========
    if (rank == 0) {
        rows = atoi(argv[1]);
        cols = atoi(argv[2]);
        // 矩阵乘法要求：A的列数 == B的行数
        if (rows != cols) {
            cerr << "Warning: For C=A×B with A,B both " << rows << "×" << cols
                << ", using square matrix assumption (n=" << rows << ")." << endl;
        }
        for (int i = 1; i < size; ++i) {
            MPI_Send(&rows, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&cols, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    }
    else {
        MPI_Recv(&rows, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&cols, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // 使用方阵维度 n（假设 rows×rows 矩阵乘法）
    int n = rows;  // 方阵维度

    // ========== 2. 负载均衡：A按行分块 ==========
    int base_rows = n / size;
    int remainder = n % size;
    int my_rows = base_rows + (rank == size - 1 ? remainder : 0);  // 当前进程处理的A的行数

    // ========== 3. 主进程生成数据 ==========
    vector<vector<int>> mat1, mat2;  // 仅用于小矩阵打印
    vector<int> flatA, flatB;        // 展平的一维数组

    if (rank == 0) {
        mat1 = generateRandomMatrix(n, n, 0, 100);  // A: n×n
        mat2 = generateRandomMatrix(n, n, 0, 100);  // B: n×n

        // 展平 A 和 B（行优先）
        flatA.resize(n * n);
        flatB.resize(n * n);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                flatA[i * n + j] = mat1[i][j];
                flatB[i * n + j] = mat2[i][j];
            }
        }

        // 条件打印输入矩阵
        if (n < 6) {
            printMatrix(mat1, "Matrix A");
            printMatrix(mat2, "Matrix B");
        }
    }

    // ========== 4. 分配局部缓冲区 ==========
    vector<int> localA(my_rows * n);        // 当前进程的 A 块: my_rows × n
    vector<int> fullB(n * n);               // 完整的 B 矩阵: n × n（所有进程都需要）
    vector<int> localC(my_rows * n);        // 当前进程的结果 C 块: my_rows × n

    // ========== 5. 数据分发 ==========
    // 5.1 分发矩阵 A（按行分块）
    if (rank == 0) {
        // 主进程复制自己的 A 块
        int my_offset = 0;
        for (int i = 0; i < my_rows * n; ++i) {
            localA[i] = flatA[my_offset + i];
        }
        // 向其他进程发送 A 块
        int offset = base_rows * n;
        for (int p = 1; p < size; ++p) {
            int proc_rows = base_rows + (p == size - 1 ? remainder : 0);
            int count = proc_rows * n;
            MPI_Send(flatA.data() + offset, count, MPI_INT, p, 1, MPI_COMM_WORLD);  // tag=1: A数据
            offset += count;
        }
    }
    else {
        MPI_Recv(localA.data(), my_rows * n, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // 5.2 广播矩阵 B（完整广播）
    if (rank == 0) {
        // 主进程保留自己的 B
        fullB = flatB;
        // 向其他进程发送完整的 B
        for (int p = 1; p < size; ++p) {
            MPI_Send(flatB.data(), n * n, MPI_INT, p, 3, MPI_COMM_WORLD);  // tag=3: B数据
        }
    }
    else {
        MPI_Recv(fullB.data(), n * n, MPI_INT, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // ========== 6. 局部矩阵乘法计算（计时）==========
    // C[i][j] = Σ A[i][k] * B[k][j], k=0~n-1
    auto start = high_resolution_clock::now();

    for (int i = 0; i < my_rows; ++i) {           // 当前进程的行
        for (int j = 0; j < n; ++j) {             // 结果矩阵的列
            int sum = 0;
            for (int k = 0; k < n; ++k) {         // 累加维度
                sum += localA[i * n + k] * fullB[k * n + j];
            }
            localC[i * n + j] = sum;
        }
    }

    auto stop = high_resolution_clock::now();

    // ========== 7. 结果收集（按行 Gather）==========
    if (rank == 0) {
        vector<int> globalC(n * n);

        // 复制自己的结果块
        for (int i = 0; i < my_rows * n; ++i) {
            globalC[i] = localC[i];
        }

        // 接收其他进程的结果
        int offset = base_rows * n;
        for (int p = 1; p < size; ++p) {
            int proc_rows = base_rows + (p == size - 1 ? remainder : 0);
            int count = proc_rows * n;
            MPI_Recv(globalC.data() + offset, count, MPI_INT, p, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  // tag=2: 结果
            offset += count;
        }

        // 条件打印结果矩阵
        if (n < 6) {
            vector<vector<int>> resultMatrix(n, vector<int>(n));
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    resultMatrix[i][j] = globalC[i * n + j];
                }
            }
            printMatrix(resultMatrix, "Result Matrix (A × B)");
        }

        // 输出耗时
        auto duration = duration_cast<microseconds>(stop - start);
        cout << "Time taken by matrix multiplication: " << duration.count() << " microseconds" << endl;

    }
    else {
        // worker 进程发送结果回主进程
        MPI_Send(localC.data(), my_rows * n, MPI_INT, 0, 2, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}