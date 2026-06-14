/**
 * MPI Parallel Sobel Edge Detection
 * 功能：使用 MPI 多进程并行加速 Sobel 边缘检测
 * 编译：mpicxx -o edge_detect edge_detect.cpp `pkg-config --cflags --libs
 * opencv4` 运行：mpiexec -n 4 ./edge_detect
 */

#include <cstring>
#include <iostream>
#include <mpi.h>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using namespace cv;

// ================= 常量定义 =================
const int KERNEL_SIZE = 3;
const int KERNEL_DATA[] = {-1, 0, 1,
                           -2, 0, 2, // Sobel X 方向算子（检测垂直边缘）
                           -1, 0, 1};

// 调试宏：带进程编号的输出，强制刷新缓冲区避免乱序
#define DEBUG_PRINT(msg)                                                       \
  do {                                                                         \
    std::ostringstream oss;                                                    \
    oss << "[Rank " << rank << "] " << msg;                                    \
    std::cout << oss.str() << std::endl;                                       \
    std::cout.flush();                                                         \
    fflush(stdout);                                                            \
  } while (0)

// ================= 主函数 =================
int main(int argc, char **argv) {
  // ----- 1. MPI 初始化 -----
  MPI_Init(&argc, &argv);
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  double start_time = 0.0, end_time = 0.0;
  int width = 0, height = 0;
  int error_code = 0;

  uchar *full_data = nullptr;     // Rank 0: 完整图像数据
  uchar *local_buffer = nullptr;  // 所有进程: 本地数据块（含 Halo）
  uchar *result_buffer = nullptr; // 所有进程: 本地计算结果
  Mat gray;                       // Rank 0: 灰度图

  // ----- 2. Rank 0 读取图像 + 错误码广播 -----
  if (rank == 0) {
    string imagePath = "C:/teaching/test1.jpg";
    Mat src = imread(imagePath);

    if (src.empty()) {
      DEBUG_PRINT("错误：无法加载图像 " << imagePath);
      error_code = -1;
    } else {
      cvtColor(src, gray, COLOR_BGR2GRAY);
      width = gray.cols;
      height = gray.rows;
      full_data = gray.data;
      DEBUG_PRINT("图像加载成功: " << width << "x" << height);
    }
  }

  // 关键：所有进程必须调用 MPI_Bcast（集体通信）
  MPI_Bcast(&error_code, 1, MPI_INT, 0, MPI_COMM_WORLD);
  if (error_code != 0) {
    DEBUG_PRINT("收到错误信号，安全退出");
    MPI_Finalize();
    return -1;
  }

  // 关键：图像尺寸广播（所有进程参与）
  MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);

  if (rank != 0) {
    DEBUG_PRINT("收到图像尺寸: " << width << "x" << height);
  }

  // ----- 3. 负载均衡计算（行分块策略） -----
  vector<int> sendcounts(size); // 每个进程的数据量（字节数）
  vector<int> displs(size);     // 每个数据块在完整数组中的偏移

  int base_rows = height / size;
  int remainder = height % size; // 余数行，前 remainder 个进程多分1行

  for (int i = 0; i < size; i++) {
    int rows_i = base_rows + (i < remainder ? 1 : 0);
    sendcounts[i] = rows_i * width; // 字节数 = 行数 × 列数
    displs[i] = (i == 0) ? 0 : (displs[i - 1] + sendcounts[i - 1]);
  }

  int local_rows = sendcounts[rank] / width; // 本进程负责的有效行数
  DEBUG_PRINT("本地分配: " << local_rows << " 行, 数据量: " << sendcounts[rank]
                           << " 字节");

  // ----- 4. 内存分配（含上下 Halo 区） -----
  // 布局: [Top Halo:1行] + [有效数据:local_rows行] + [Bottom Halo:1行]
  int local_buffer_size = (local_rows + 2) * width;
  local_buffer = new uchar[local_buffer_size];
  result_buffer = new uchar[local_buffer_size];
  memset(local_buffer, 0, local_buffer_size);
  memset(result_buffer, 0, local_buffer_size);

  // ----- 5. 数据分发: MPI_Scatterv -----
  // 注意: 数据写入位置跳过 Top Halo (local_buffer + width)
  DEBUG_PRINT("开始 MPI_Scatterv 数据分发...");
  MPI_Scatterv(full_data,                        // Rank 0: 源数据指针
               sendcounts.data(), displs.data(), // 各进程数据量 + 偏移
               MPI_UNSIGNED_CHAR,                // 数据类型
               local_buffer + width, // ⚠️ 接收缓冲区偏移（预留 Top Halo）
               sendcounts[rank], MPI_UNSIGNED_CHAR, 0,
               MPI_COMM_WORLD // root 进程
  );
  DEBUG_PRINT("MPI_Scatterv 完成");

  // ----- 6. Halo 边界交换（关键！保证卷积边界正确） -----
  int up_rank = rank - 1;
  int down_rank = rank + 1;

  // 6.1 与上游进程交换（获取 Top Halo）
  if (rank > 0) {
    DEBUG_PRINT("↔ 上边界交换: 与 Rank " << up_rank << " 通信");
    MPI_Sendrecv(
        // 发送：我的第1行有效数据 → 上游的 Bottom Halo
        local_buffer + width, width, MPI_UNSIGNED_CHAR, up_rank,
        100, // Tag 100: 发送有效数据行
        // 接收：上游的最后1行 → 我的 Top Halo
        local_buffer, width, MPI_UNSIGNED_CHAR, up_rank,
        101, // Tag 101: 接收 Halo 数据
        MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  } else {
    // Rank 0 无上游：复制第1行到 Top Halo（边界填充策略）
    memcpy(local_buffer, local_buffer + width, width * sizeof(uchar));
    DEBUG_PRINT("Rank 0: 上边界使用复制填充");
  }

  // 6.2 与下游进程交换（获取 Bottom Halo）
  if (rank < size - 1) {
    DEBUG_PRINT("↔ 下边界交换: 与 Rank " << down_rank << " 通信");
    uchar *my_last_row =
        local_buffer + width + (local_rows - 1) * width; // 我的最后有效行
    uchar *my_bottom_halo =
        local_buffer + (local_rows + 1) * width; // 我的 Bottom Halo 位置

    MPI_Sendrecv(
        // 发送：我的最后1行有效数据 → 下游的 Top Halo
        my_last_row, width, MPI_UNSIGNED_CHAR, down_rank, 101, // Tag 101
        // 接收：下游的第1行 → 我的 Bottom Halo
        my_bottom_halo, width, MPI_UNSIGNED_CHAR, down_rank, 100, // Tag 100
        MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  } else {
    // 最后一个进程无下游：复制最后1行到 Bottom Halo
    uchar *last_row = local_buffer + width + (local_rows - 1) * width;
    uchar *bottom_halo = local_buffer + (local_rows + 1) * width;
    memcpy(bottom_halo, last_row, width * sizeof(uchar));
    DEBUG_PRINT("Rank " << size - 1 << ": 下边界使用复制填充");
  }
  DEBUG_PRINT(" Halo 交换全部完成");

  // ----- 7. 本地卷积计算（并行核心） -----
  // 包装缓冲区为 OpenCV Mat（含 Halo 区，共 local_rows+2 行）
  Mat local_mat(local_rows + 2, width, CV_8UC1, local_buffer);
  Mat local_res(local_rows + 2, width, CV_8UC1, result_buffer);
  Mat kernel(KERNEL_SIZE, KERNEL_SIZE, CV_32S, (void *)KERNEL_DATA);

  Mat temp_res;
  // CV_16S: 使用16位有符号整数防止卷积溢出（Sobel结果可能为负或>255）
  filter2D(local_mat, temp_res, CV_16S, kernel);
  // 转回 uint8 并取绝对值
  convertScaleAbs(temp_res, local_res);
  DEBUG_PRINT("本地卷积计算完成");

  // ----- 8. 结果收集: MPI_Gatherv -----
  // Rank 0 预先分配接收缓冲区
  if (rank == 0) {
    full_data = new uchar[width * height];
    start_time = MPI_Wtime(); // 计时起点（包含通信+计算）
  }

  DEBUG_PRINT("开始 MPI_Gatherv 结果收集...");
  // 注意: 发送时跳过 Top Halo，只发送有效结果 (result_buffer + width)
  MPI_Gatherv(result_buffer + width, // ⚠️ 发送指针偏移（跳过 Top Halo）
              sendcounts[rank], MPI_UNSIGNED_CHAR,
              full_data, // Rank 0: 接收完整结果
              sendcounts.data(), displs.data(), MPI_UNSIGNED_CHAR, 0,
              MPI_COMM_WORLD);
  DEBUG_PRINT("MPI_Gatherv 完成");

  // ----- 9. Rank 0 保存结果 + 性能统计 -----
  if (rank == 0) {
    end_time = MPI_Wtime();
    double elapsed_ms = (end_time - start_time) * 1000.0;
    DEBUG_PRINT("并行处理耗时: " << elapsed_ms << " ms");

    // 包装结果数据为 Mat 并保存
    Mat dst(height, width, CV_8UC1, full_data);
    string outputPath = "C:/teaching/result_edge.jpg";
    bool success = imwrite(outputPath, dst);

    if (success) {
      DEBUG_PRINT("结果已保存: " << outputPath);
    } else {
      DEBUG_PRINT("保存失败，请检查路径权限");
    }

    delete[] full_data;
    gray.release();
  }

  // ----- 10. 资源清理 -----
  delete[] local_buffer;
  delete[] result_buffer;

  DEBUG_PRINT("ank " << rank << " 正常退出");
  MPI_Finalize(); // 必须调用：释放 MPI 资源
  return 0;
}