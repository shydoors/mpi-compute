/**
 * 树型归约
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int my_val = rank + 1;          // 每个进程的初始数据
    int left  = 2 * rank + 1;       // 左子节点
    int right = 2 * rank + 2;       // 右子节点
    int parent = (rank - 1) / 2;    // 父节点（根节点无父节点）
    MPI_Status status;
    int recv_val, total = my_val;
    int count;
    // 如果有子节点，则等待接收子进程消息
    if (left < size) {
        MPI_Recv(&recv_val, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_INT, &count); // 获取实际接收数据量（演示 MPI_Status 用法）
        printf("进程 %d 从源 %d 接收消息 (tag=%d, count=%d)\n", rank, status.MPI_SOURCE, status.MPI_TAG, count);
        total += recv_val;
        if (right < size) { // 存在右子节点
            MPI_Recv(&recv_val, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            total += recv_val;
        }
    }
    // 非根节点将累加结果发送给父节点
    if (rank != 0) {
        MPI_Send(&total, 1, MPI_INT, parent, 99, MPI_COMM_WORLD);
    } else {
        printf("树型归约完成 根进程 %d 收到最终结果: %d\n", rank, total);
    }
    MPI_Finalize();
    return 0;
}