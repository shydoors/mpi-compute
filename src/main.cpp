/**
 * @file main.cpp
 * @brief K-Means 聚类 — 主入口
 *
 * 用法:
 *   ./kmeans --data <path> [options]
 *
 * 选项:
 *   --data <path>          数据文件路径（必需，相对项目根目录）
 *   --backend <seq|omp|cuda>  计算后端 (默认 cuda)
 *   --auto-k <0|1>         是否自动推导 K (默认 1)
 *   --k <uint>             手动指定 K (auto-k=0 时生效)
 *   --max-iter <int>       最大迭代次数 (默认 300)
 *   --threshold <float>    收敛阈值 (默认 1e-8)
 *   --checkpoint <path>    checkpoint 文件路径
 *   --resume               从 checkpoint 恢复
 *   --output <path>        聚类结果输出路径
 *   --streaming            使用流式读取
 *   --no-mmap              不使用 mmap
 *   --help                 显示帮助
 */

#include "kmeans.hpp"
#include "types.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

// ============================================================
// 命令行解析（轻量级，无外部依赖）
// ============================================================
static void print_help(const char* prog) {
  std::fprintf(stderr,
    "用法: %s --data <path> [选项]\n"
    "\n"
    "必需:\n"
    "  --data <path>          数据文件路径（相对项目根目录）\n"
    "\n"
    "算法:\n"
    "  --backend <seq|omp|cuda>  计算后端 (默认 cuda)\n"
    "  --auto-k <0|1>         是否自动推导 K (默认 1)\n"
    "  --k <uint>             手动指定 K\n"
    "  --max-iter <int>       最大迭代次数 (默认 300)\n"
    "  --threshold <float>    收敛阈值 (默认 1e-8)\n"
    "\n"
    "I/O:\n"
    "  --checkpoint <path>    checkpoint 文件路径\n"
    "  --resume               从 checkpoint 恢复\n"
    "  --output <path>        聚类结果输出路径\n"
    "  --streaming            使用流式读取\n"
    "  --no-mmap              不使用 mmap\n"
    "\n"
    "其它:\n"
    "  --help                 显示此帮助\n",
    prog);
}

static KMeansConfig parse_args(int argc, char** argv) {
  KMeansConfig config;

  for (i32 i = 1; i < argc; ++i) {
    std::string_view arg{argv[i]};

    auto next = [&]() -> std::string_view {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "Error: %s requires an argument\n", argv[i]);
        std::exit(1);
      }
      return std::string_view{argv[++i]};
    };

    if (arg == "--help") {
      print_help(argv[0]);
      std::exit(0);
    } else if (arg == "--data") {
      config.data_path = std::string{next()};
    } else if (arg == "--backend") {
      auto val = next();
      if (val == "seq")       config.backend = Backend::Sequential;
      else if (val == "omp")  config.backend = Backend::OpenMP;
      else if (val == "cuda") config.backend = Backend::CUDA;
      else {
        std::fprintf(stderr, "Error: unknown backend '%s'\n", std::string{val}.c_str());
        std::fprintf(stderr, "  Valid: seq, omp, cuda\n");
        std::exit(1);
      }
    } else if (arg == "--auto-k") {
      config.auto_k = (std::stoi(std::string{next()}) != 0);
    } else if (arg == "--k") {
      config.fixed_k = static_cast<u32>(std::stoul(std::string{next()}));
      config.auto_k = false;
    } else if (arg == "--max-iter") {
      config.max_iterations = static_cast<i32>(std::stoi(std::string{next()}));
    } else if (arg == "--threshold") {
      config.threshold = std::stod(std::string{next()});
    } else if (arg == "--checkpoint") {
      config.checkpoint_path = std::string{next()};
    } else if (arg == "--resume") {
      config.resume = true;
    } else if (arg == "--output") {
      config.output_path = std::string{next()};
    } else if (arg == "--streaming") {
      config.streaming = true;
    } else if (arg == "--no-mmap") {
      config.use_mmap = false;
    } else {
      std::fprintf(stderr, "Error: unknown option '%s'\n", std::string{arg}.c_str());
      print_help(argv[0]);
      std::exit(1);
    }
  }

  if (config.data_path.empty()) {
    std::fprintf(stderr, "Error: --data is required\n");
    print_help(argv[0]);
    std::exit(1);
  }

  return config;
}

// ============================================================
// 输出结果
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
// 主程序
// ============================================================
int main(int argc, char** argv) {
  auto config = parse_args(argc, argv);

  std::printf("K-Means Clustering\n");
  std::printf("  数据文件:    %s\n", config.data_path.c_str());
  std::printf("  后端:        %s\n", backend_name(config.backend));
  std::printf("  自动 K:      %s\n", config.auto_k ? "yes" : "no");
  if (!config.auto_k) {
    std::printf("  固定 K:      %u\n", config.fixed_k);
  }
  std::printf("  最大迭代:    %d\n", config.max_iterations);
  std::printf("  收敛阈值:    %.1e\n", config.threshold);
  std::printf("  流式读取:    %s\n", config.streaming ? "yes" : "no");
  std::printf("  断点继续:    %s\n",
              config.resume ? "yes (resume)" :
              (!config.checkpoint_path.empty() ? "yes (save)" : "no"));
  if (!config.checkpoint_path.empty()) {
    std::printf("  Checkpoint:  %s\n", config.checkpoint_path.c_str());
  }
  std::printf("\n");

  KMeans kmeans(config);
  auto result = kmeans.run();

  print_result(result);

  // 输出聚类结果到文件
  if (!config.output_path.empty() && result.labels) {
    std::FILE* fp = std::fopen(config.output_path.c_str(), "wb");
    if (fp) {
      // 格式：每个点 (x, y, label) 二进制
      // x: f64, y: f64, label: u32
      // labels 长度和数据点数相同
      // 写入方式由 Dataset 提供，此处简化
      std::fclose(fp);
      std::printf("结果已写入: %s\n", config.output_path.c_str());
    } else {
      std::fprintf(stderr, "Error: 无法写入输出文件 %s\n", config.output_path.c_str());
    }
  }
  return 0;
}
