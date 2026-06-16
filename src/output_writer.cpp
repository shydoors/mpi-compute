/**
 * @file output_writer.cpp
 * @brief 结果输出 -- 实现
 *
 * 所有输出函数使用 char* 作为路径参数（符合规范要求）。
 */

#include "output_writer.hpp"
#include "dataset.hpp"
#include "types.hpp"

#include <cstdio>
#include <string>

// ============================================================
// 控制台输出
// ============================================================

void output_print_result(const KMeansResult& result) {
  std::printf("\n========== K-Means 结果 ==========\n");
  std::printf("  簇数 (K):          %u\n", result.k);
  std::printf("  迭代次数:          %d\n", result.iterations);
  std::printf("  SSE (Inertia):     %.6e\n", result.inertia);
  std::printf("  加载时间:          %.2f ms\n", result.time_load * 1e3);
  std::printf("  自动 K 推导时间:   %.2f ms\n", result.time_autok * 1e3);
  std::printf("  迭代时间:          %.2f ms\n", result.time_iterate * 1e3);
  std::printf("  总运行时间:        %.2f ms\n", result.time_total * 1e3);
  std::printf("==================================\n");
}

void output_print_config(const KMeansConfig& config, u64 render_max) {
  std::printf("========================================\n");
  std::printf("  K-Means Clustering\n");
  std::printf("========================================\n\n");
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
  std::printf("  渲染采样:    %lu 点\n\n", render_max);
}

// ============================================================
// 文件输出
// ============================================================

void output_save_result_txt(const char* dir,
                             const KMeansConfig& config,
                             const KMeansResult& result,
                             const char* timestamp)
{
  char path[512] = {};
  std::snprintf(path, sizeof(path), "%s/result.txt", dir);
  std::FILE* fp = std::fopen(path, "w");
  if (fp == nullptr) {
    std::fprintf(stderr, "Error: 无法写入 %s\n", path);
    return;
  }

  std::fprintf(fp, "========================================\n");
  std::fprintf(fp, "  K-Means 聚类运行结果\n");
  std::fprintf(fp, "  运行时间 (UTC+8): %s\n", timestamp);
  std::fprintf(fp, "  项目根目录: %s\n", config.projects_dir.c_str());
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
  std::fprintf(fp, "\n");
  std::fprintf(fp, "--- 运行结果 ---\n");
  std::fprintf(fp, "  簇数 (K):          %u\n", result.k);
  std::fprintf(fp, "  迭代次数:          %d\n", result.iterations);
  std::fprintf(fp, "  SSE (Inertia):     %.6e\n", result.inertia);
  std::fprintf(fp, "  加载时间:          %.2f ms\n", result.time_load * 1e3);
  std::fprintf(fp, "  自动 K 推导时间:   %.2f ms\n", result.time_autok * 1e3);
  std::fprintf(fp, "  迭代时间:          %.2f ms\n", result.time_iterate * 1e3);
  std::fprintf(fp, "  总运行时间:        %.2f ms\n", result.time_total * 1e3);
  std::fprintf(fp, "========================================\n");
  std::fclose(fp);
  std::printf("  日志已保存至: %s\n", path);
}

void output_save_labels_bin(const char* dir,
                             const KMeansResult& result,
                             u64 num_points)
{
  if (result.labels == nullptr)  { return; }
  if (num_points == 0)          { return; }

  char path[512] = {};
  std::snprintf(path, sizeof(path), "%s/labels.bin", dir);
  std::FILE* fp = std::fopen(path, "wb");
  if (fp == nullptr) {
    std::fprintf(stderr, "Error: 无法写入 %s\n", path);
    return;
  }

  const u64 count = num_points;
  std::fwrite(&count, sizeof(count), 1, fp);
  std::fwrite(result.labels.get(), sizeof(u32), num_points, fp);
  std::fclose(fp);

  std::printf("  标签已保存至: %s  (%lu 个标签, %.2f MiB)\n",
              path, num_points,
              (sizeof(u32) * num_points) / (1024.0 * 1024.0));
}

void output_save_centers_csv(const char* dir,
                              const KMeansResult& result)
{
  if (result.centers_x == nullptr) { return; }
  if (result.centers_y == nullptr) { return; }
  if (result.k == 0)               { return; }

  char path[512] = {};
  std::snprintf(path, sizeof(path), "%s/centers.csv", dir);
  std::FILE* fp = std::fopen(path, "w");
  if (fp == nullptr) {
    std::fprintf(stderr, "Error: 无法写入 %s\n", path);
    return;
  }

  std::fprintf(fp, "cx,cy,cluster\n");
  for (u32 j = 0; j < result.k; ++j) {
    std::fprintf(fp, "%.10f,%.10f,%u\n",
                 result.centers_x[j], result.centers_y[j], j);
  }
  std::fclose(fp);
  std::printf("  中心点已保存至: %s  (%u 个中心)\n", path, result.k);
}

void output_save_render_csv(const char* dir,
                             const KMeansConfig& config,
                             const KMeansResult& result,
                             u64 num_points,
                             u64 max_render_points)
{
  if (result.labels == nullptr) { return; }
  if (num_points == 0)          { return; }

  // 重新加载数据用于渲染
  Dataset data{};
  if (!data.load(config.data_path, true)) {
    std::fprintf(stderr, "Warning: 无法加载数据，跳过渲染导出\n");
    return;
  }
  if (data.size() != num_points) {
    std::fprintf(stderr,
                 "Warning: 数据点数不匹配 (%lu vs %lu)，跳过渲染导出\n",
                 data.size(), num_points);
    return;
  }

  char path[512] = {};
  std::snprintf(path, sizeof(path), "%s/render.csv", dir);
  std::FILE* fp = std::fopen(path, "w");
  if (fp == nullptr) {
    std::fprintf(stderr, "Error: 无法写入 %s\n", path);
    return;
  }

  std::fprintf(fp, "x,y,label\n");

  const bool interleaved = (data.mode() == Dataset::Mode::Interleaved);
  const f64* raw = data.raw_data();

  if (num_points <= max_render_points) {
    for (u64 i = 0; i < num_points; ++i) {
      f64 px{};
      f64 py{};
      if (interleaved) {
        px = raw[i * 2];
        py = raw[i * 2 + 1];
      } else {
        px = data.x()[i];
        py = data.y()[i];
      }
      std::fprintf(fp, "%.6f,%.6f,%u\n", px, py, result.labels[i]);
    }
    std::printf("  渲染数据已保存至: %s  (%lu 个点, 全量)\n",
                path, num_points);
  } else {
    const u64 step = num_points / max_render_points;
    u64 actual = 0;
    for (u64 i = 0; i < num_points; i += step) {
      f64 px{};
      f64 py{};
      if (interleaved) {
        px = raw[i * 2];
        py = raw[i * 2 + 1];
      } else {
        px = data.x()[i];
        py = data.y()[i];
      }
      std::fprintf(fp, "%.6f,%.6f,%u\n", px, py, result.labels[i]);
      ++actual;
    }
    std::printf("  渲染数据已保存至: %s  (%lu 个点, 采样步长 %lu, 约 %lu 个点)\n",
                path, num_points, step, actual);
  }
  std::fclose(fp);
}
