/**
 * @file main.cpp
 * @brief K-Means 聚类 — 主入口（函数式调用，VS2019 直接运行版）
 *
 * 使用方法（VS2019）:
 *   打开解决方案 → 按 F5（调试运行）或 Ctrl+F5（直接运行）即可。
 *   所有参数在下方 kmeans_run() 开头的变量中设置，无需命令行传参。
 *
 * 运行结果自动保存到 result_log/ 文件夹，以时间戳命名。
 */

#include "kmeans.hpp"
#include "types.hpp"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>

// ============================================================
// 保存结果到文件
// ============================================================
static void save_result_to_file(const KMeansConfig& config,
                                const KMeansResult& result,
                                const char* timestamp) {
  std::string dir = "result_log";
  std::string filename = dir + "/result_" + timestamp + ".txt";

  // 创建目录（兼容 Windows 和 Linux）
#ifdef _WIN32
  std::system(("if not exist " + dir + " mkdir " + dir).c_str());
#else
  std::system(("mkdir -p " + dir).c_str());
#endif

  std::FILE* fp = std::fopen(filename.c_str(), "w");
  if (!fp) {
    std::fprintf(stderr, "Error: 无法创建结果文件 %s\n", filename.c_str());
    return;
  }

  std::fprintf(fp, "========================================\n");
  std::fprintf(fp, "  K-Means 聚类运行结果\n");
  std::fprintf(fp, "  运行时间: %s\n", timestamp);
  std::fprintf(fp, "========================================\n\n");

  std::fprintf(fp, "--- 配置参数 ---\n");
  std::fprintf(fp, "  数据文件:        %s\n", config.data_path.c_str());
  std::fprintf(fp, "  计算后端:        %s\n", backend_name(config.backend));
  std::fprintf(fp, "  自动推导 K:      %s\n", config.auto_k ? "是" : "否");
  if (!config.auto_k) {
    std::fprintf(fp, "  指定 K:          %u\n", config.fixed_k);
  }
  std::fprintf(fp, "  最大迭代次数:    %d\n", config.max_iterations);
  std::fprintf(fp, "  收敛阈值:        %.1e\n", config.threshold);
  std::fprintf(fp, "  流式读取:        %s\n", config.streaming ? "是" : "否");
  std::fprintf(fp, "  断点继续:        %s\n",
               config.resume ? "是 (恢复)" :
               (!config.checkpoint_path.empty() ? "是 (保存)" : "否"));
  if (!config.checkpoint_path.empty()) {
    std::fprintf(fp, "  Checkpoint 路径: %s\n", config.checkpoint_path.c_str());
  }
  std::fprintf(fp, "\n");
  std::fprintf(fp, "--- 运行结果 ---\n");
  std::fprintf(fp, "  簇数 (K):          %u\n", result.k);
  std::fprintf(fp, "  迭代次数:          %d\n", result.iterations);
  std::fprintf(fp, "  SSE (Inertia):     %.6e\n", result.inertia);
  std::fprintf(fp, "  加载时间:          %.3f s\n", result.time_load);
  std::fprintf(fp, "  自动 K 推导时间:   %.3f s\n", result.time_auto_k);
  std::fprintf(fp, "  迭代时间:          %.3f s\n", result.time_iterate);
  std::fprintf(fp, "  总运行时间:        %.3f s\n", result.time_total);
  std::fprintf(fp, "\n");
  std::fprintf(fp, "========================================\n");
  std::fclose(fp);
  std::printf("  结果已保存至: %s\n", filename.c_str());
}

// ============================================================
// 控制台输出结果
// ============================================================
static void print_result(const KMeansResult& result) {
  std::printf("\n========== K-Means 结果 ==========\n");
  std::printf("  簇数 (K):          %u\n", result.k);
  std::printf("  迭代次数:          %d\n", result.iterations);
  std::printf("  SSE (Inertia):     %.6e\n", result.inertia);
  std::printf("  加载时间:          %.3f s\n", result.time_load);
  std::printf("  自动 K 推导时间:   %.3f s\n", result.time_auto_k);
  std::printf("  迭代时间:          %.3f s\n", result.time_iterate);
  std::printf("  总运行时间:        %.3f s\n", result.time_total);
  std::printf("==================================\n");
}

// ============================================================
// 函数式入口 — 直接修改下方变量即可切换配置
// ============================================================
static void kmeans_run() {
  // ==========================================================
  //  【在这里修改参数】
  //  改完直接按 F5 运行
  // ==========================================================

  // --- 数据文件路径（相对项目根目录） ---
  std::string data_path = "data/a.dat";

  // --- 计算后端 ---
  // 可选值: "seq"  (串行)
  //         "omp"  (OpenMP 多核)
  //         "cuda" (GPU 加速)
  std::string backend = "seq";

  // --- 簇数 K ---
  // true  = 自动推导最优 K（Kneedle 肘部检测）
  // false = 使用下方 fixed_k 指定的值
  bool auto_k = false;

  // 手动指定 K（仅在 auto_k = false 时生效）
  u32 fixed_k = 5;

  // --- 迭代控制 ---
  i32 max_iter    = 30;     // 最大迭代次数
  f64 threshold   = 1e-4;   // 收敛阈值（中心移动量小于此值时停止）

  // --- 读取方式 ---
  bool streaming  = false;  // true = 流式读取（内存不足时使用）

  // --- 断点继续 ---
  std::string checkpoint_path = "";  // 非空则启用 checkpoint 功能
  bool resume   = false;             // true = 从 checkpoint 恢复

  // ==========================================================
  //  以下代码一般无需修改
  // ==========================================================

  std::printf("========================================\n");
  std::printf("  K-Means Clustering\n");
  std::printf("========================================\n\n");

  // 构建配置对象
  KMeansConfig config;
  config.data_path = data_path;

  if (backend == "seq")       config.backend = Backend::Sequential;
  else if (backend == "omp")  config.backend = Backend::OpenMP;
  else if (backend == "cuda") config.backend = Backend::CUDA;
  else {
    std::fprintf(stderr, "Error: 未知后端 '%s'，使用 seq\n", backend.c_str());
    config.backend = Backend::Sequential;
  }

  config.auto_k        = auto_k;
  config.fixed_k       = fixed_k;
  config.max_iterations = max_iter;
  config.threshold     = threshold;
  config.streaming     = streaming;
  config.checkpoint_path = checkpoint_path;
  config.resume        = resume;
  config.output_path   = "";  // 结果保存由本函数统一处理

  // 打印配置
  std::printf("  数据文件:    %s\n", config.data_path.c_str());
  std::printf("  后端:        %s\n", backend_name(config.backend));
  std::printf("  自动 K:      %s\n", config.auto_k ? "是" : "否");
  if (!config.auto_k) {
    std::printf("  固定 K:      %u\n", config.fixed_k);
  }
  std::printf("  最大迭代:    %d\n", config.max_iterations);
  std::printf("  收敛阈值:    %.1e\n", config.threshold);
  std::printf("  流式读取:    %s\n", config.streaming ? "是" : "否");
  std::printf("  断点继续:    %s\n",
              config.resume ? "是 (恢复)" :
              (!config.checkpoint_path.empty() ? "是 (保存)" : "否"));
  if (!config.checkpoint_path.empty()) {
    std::printf("  Checkpoint:  %s\n", config.checkpoint_path.c_str());
  }
  std::printf("\n");

  // 运行 K-Means
  KMeans kmeans(config);
  auto result = kmeans.run();

  // 控制台输出结果
  print_result(result);

  // 保存结果到文件
  auto now    = std::chrono::system_clock::now();
  auto tt     = std::chrono::system_clock::to_time_t(now);
  char timestamp[64];
  std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&tt));
  save_result_to_file(config, result, timestamp);

  std::printf("\n运行完毕！按 Enter 键退出...\n");
  std::getchar();
}
// ============================================================
// 主函数
// ============================================================
int main() {
  kmeans_run();
  return 0;
}
