#include <cstdio>
/**
 * @file main.cpp
 * @brief K-Means 聚类 — 主入口（读 config.json，基于 JsonConfig 解析）
 *
 * 使用方法:
 *   1. 修改 config.json 中的运行参数
 *   2. 编译运行：./build/kmeans（或 VS2019 按 F5）
 *     改 config.json 无需重新编译。
 *
 * 路径解析规则:
 *   程序启动时自动检测项目根目录（从 CWD 向上查找含 data/ 和 src/ 的目录）。
 *   config.json 中的所有相对路径字段（data_path, checkpoint_path 等）
 *   都会基于 ProjectsDir 拼接为绝对路径后再传给 K-Means 核心。
 *
 *   ProjectsDir 也可以在 config.json 中手动指定(绝对路径/相对路径均可)，
 *   此时跳过自动检测。
 *
 * 输出目录（results/ 下以 UTC+8 时间分文件夹）:
 *   results/YYYYMMDD_HHMMSS/
 *     ├── result.txt         — 运行日志
 *     ├── labels.bin         — 所有点的聚类标签（二进制，不丢失数据）
 *     ├── centers.csv        — 各簇中心坐标
 *     └── render.csv         — 渲染用采样点
 */

#include "json_config.hpp"
#include "kmeans.hpp"
#include "types.hpp"
#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

// ============================================================
// 路径工具
// ============================================================
namespace {

static bool is_absolute_path(const std::string& path) {
  if (path.empty()) return false;
  if (path[0] == '/') return true;
#ifdef _WIN32
  if (path.size() >= 2 && std::isalpha(path[0]) && path[1] == ':') return true;
#endif
  return false;
}

static std::string join_path(const std::string& base, const std::string& rel) {
  if (rel.empty()) {return base;}
  if (is_absolute_path(rel)){ return rel;}
  if (base.empty()) {return rel;}
  std::string result = base;
  if (result.back() != '/'){ result += '/';}
  result += rel;
  return result;
}

static std::string resolve_path(const std::string& path, const std::string& base) {
  if (path.empty()) {return path;}
  if (is_absolute_path(path)) {return path;}
  return join_path(base, path);
}

static std::string detect_project_dir() {
  char buf[PATH_MAX];
  if (!::getcwd(buf, sizeof(buf))) return "";
  std::string dir(buf);
  while (true) {
    bool has_data = (access((dir + "/data").c_str(), F_OK) == 0);
    bool has_src  = (access((dir + "/src").c_str(),  F_OK) == 0);
    if (has_data && has_src) return dir;
    if (dir == "/" || dir.empty()) return "";
    auto pos = dir.rfind('/');
    if (pos == std::string::npos) return "";
    dir = (pos == 0) ? "/" : dir.substr(0, pos);
  }
}

static void ensure_dir(const std::string& dir) {
#ifdef _WIN32
  std::system(("if not exist \"" + dir + "\" mkdir \"" + dir + "\"").c_str());
#else
  std::system(("mkdir -p \"" + dir + "\"").c_str());
#endif
}

/// 获取 UTC+8 时间戳字符串 YYYYMMDD_HHMMSS
static std::string get_timestamp_utc8() {
  auto now = std::chrono::system_clock::now();
  auto tt  = std::chrono::system_clock::to_time_t(now);

  // UTC+8
  tt += 8 * 3600;

  struct tm t;
  gmtime_r(&tt, &t);  // gmtime_r 把 time_t 按 UTC 分解，tt 已经是 UTC+8 了

  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &t);
  return std::string(buf);
}

} // anonymous namespace

static KMeansConfig build_config(const JsonConfig& cfg,
                                  const std::string& projects_dir) {
  KMeansConfig config;
  config.projects_dir = projects_dir;

  // ---- data_path ----
  {
    std::string raw = cfg.get_string("data_path.value");
    if (raw.empty()) raw = cfg.get_string("data_path");
    if (raw.empty()) raw = "data/a.dat";
    config.data_path = resolve_path(raw, projects_dir);
  }

  // ---- backend ----
  {
    std::string bs = cfg.get_string("backend.value");
    if (bs.empty()) bs = cfg.get_string("backend");
    if (bs == "seq")       config.backend = Backend::Sequential;
    else if (bs == "omp")  config.backend = Backend::OpenMP;
    else if (bs == "cuda") config.backend = Backend::CUDA;
    else {
      std::fprintf(stderr, "Warning: 未知后端 '%s'，使用默认 cuda\n",
                   bs.c_str());
      config.backend = Backend::CUDA;
    }
  }

  // ---- K 值 ----
  {
    int k = cfg.get_int("k.value", 0);
    if (k <= 0) {
      config.auto_k  = true;
      config.fixed_k = 0;
    } else {
      config.auto_k  = false;
      config.fixed_k = static_cast<u32>(k);
    }
  }

  // ---- 迭代参数 ----
  config.max_iterations = cfg.get_int("max_iterations", kMaxIterations);
  config.threshold      = cfg.get_double("threshold", kConvergenceThreshold);

  // ---- 流式 ----
  config.streaming = cfg.get_bool("streaming", false);

  // ---- 断点 ----
  {
    std::string cp = cfg.get_string("checkpoint_path");
    config.checkpoint_path = resolve_path(cp, projects_dir);
  }
  config.resume = cfg.get_bool("resume", false);

  // ---- 输出 ----
  config.output_path = "";

  return config;
}

// ============================================================
// 输出文件 — 直接以固定文件名写入指定目录
// ============================================================
static void save_result_txt(const std::string& dir,
                            const KMeansConfig& config,
                            const KMeansResult& result,
                            const std::string& timestamp_utc8) {
  char path[512];
  std::snprintf(path, sizeof(path), "%s/result.txt", dir.c_str());
  std::FILE* fp = std::fopen(path, "w");
  if (!fp) { std::fprintf(stderr, "Error: 无法写入 %s\n", path); return; }

  std::fprintf(fp, "========================================\n");
  std::fprintf(fp, "  K-Means 聚类运行结果\n");
  std::fprintf(fp, "  运行时间 (UTC+8): %s\n", timestamp_utc8.c_str());
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

static void save_labels_bin(const std::string& dir,
                            const KMeansResult& result,
                            u64 num_points) {
  if (!result.labels || num_points == 0) return;
  char path[512];
  std::snprintf(path, sizeof(path), "%s/labels.bin", dir.c_str());
  std::FILE* fp = std::fopen(path, "wb");
  if (!fp) { std::fprintf(stderr, "Error: 无法写入 %s\n", path); return; }
  u64 count = num_points;
  std::fwrite(&count, sizeof(count), 1, fp);
  std::fwrite(result.labels.get(), sizeof(u32), num_points, fp);
  std::fclose(fp);
  std::printf("  标签已保存至: %s  (%lu 个标签, %.2f MiB)\n",
              path, num_points,
              (sizeof(u32) * num_points) / (1024.0 * 1024.0));
}

static void save_centers_csv(const std::string& dir,
                             const KMeansResult& result) {
  if (!result.centers_x || !result.centers_y || result.k == 0) return;
  char path[512];
  std::snprintf(path, sizeof(path), "%s/centers.csv", dir.c_str());
  std::FILE* fp = std::fopen(path, "w");
  if (!fp) { std::fprintf(stderr, "Error: 无法写入 %s\n", path); return; }
  std::fprintf(fp, "cx,cy,cluster\n");
  for (u32 j = 0; j < result.k; ++j) {
    std::fprintf(fp, "%.10f,%.10f,%u\n",
                 result.centers_x[j], result.centers_y[j], j);
  }
  std::fclose(fp);
  std::printf("  中心点已保存至: %s  (%u 个中心)\n", path, result.k);
}

static void save_render_csv(const std::string& dir,
                            const KMeansConfig& config,
                            const KMeansResult& result,
                            u64 num_points,
                            u64 max_render_points) {
  if (!result.labels || num_points == 0) return;
  Dataset data;
  if (!data.load(config.data_path, true)) {
    std::fprintf(stderr, "Warning: 无法加载数据，跳过渲染导出\n");
    return;
  }
  if (data.size() != num_points) {
    std::fprintf(stderr, "Warning: 数据点数不匹配 (%lu vs %lu)，跳过渲染导出\n",
                 data.size(), num_points);
    return;
  }
  char path[512];
  std::snprintf(path, sizeof(path), "%s/render.csv", dir.c_str());
  std::FILE* fp = std::fopen(path, "w");
  if (!fp) { std::fprintf(stderr, "Error: 无法写入 %s\n", path); return; }
  std::fprintf(fp, "x,y,label\n");
  bool interleaved = (data.mode() == Dataset::Mode::Interleaved);
  const f64* raw = data.raw_data();
  if (num_points <= max_render_points) {
    for (u64 i = 0; i < num_points; ++i) {
      f64 px = interleaved ? raw[i * 2] : data.x()[i];
      f64 py = interleaved ? raw[i * 2 + 1] : data.y()[i];
      std::fprintf(fp, "%.6f,%.6f,%u\n", px, py, result.labels[i]);
    }
    std::printf("  渲染数据已保存至: %s  (%lu 个点, 全量)\n", path, num_points);
  } else {
    u64 step = num_points / max_render_points;
    u64 actual = 0;
    for (u64 i = 0; i < num_points; i += step) {
      f64 px = interleaved ? raw[i * 2] : data.x()[i];
      f64 py = interleaved ? raw[i * 2 + 1] : data.y()[i];
      std::fprintf(fp, "%.6f,%.6f,%u\n", px, py, result.labels[i]);
      ++actual;
    }
    std::printf("  渲染数据已保存至: %s  (%lu 个点, 采样步长 %lu, 约 %lu 个点)\n",
                path, num_points, step, actual);
  }
  std::fclose(fp);
}

// ============================================================
// 控制台输出
// ============================================================
static void print_result(const KMeansResult& result) {
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

// ============================================================
// 主入口
// ============================================================
static void kmeans_run() {
  // ---- 1. 确定项目根目录 ----
  std::string projects_dir = detect_project_dir();

  // ---- 2. 定位并读取 config.json ----
  JsonConfig cfg;
  bool loaded = false;
  if (!projects_dir.empty()) {
    loaded = cfg.load(projects_dir + "/config.json");
  }
  if (!loaded) {
    loaded = cfg.load("config.json");
  }
  if (!loaded) {
    std::fprintf(stderr, "Error: 找不到 config.json\n"
                         "请确保在项目根目录下运行。\n"
                         "或在 config.json 中设置 projects_dir 为项目根目录的绝对路径。\n");
    std::getchar();
    return;
  }

  // ---- 3. 从 config.json 中读 ProjectsDir（如果有，覆盖自动检测的结果） ----
  {
    std::string cfg_dir = cfg.get_string("ProjectsDir");
    if (!cfg_dir.empty()) {
      if (!is_absolute_path(cfg_dir)) {
        char buf[PATH_MAX];
        if (::getcwd(buf, sizeof(buf))) {
          cfg_dir = join_path(std::string(buf), cfg_dir);
        }
      }
      projects_dir = cfg_dir;
    }
  }

  if (projects_dir.empty()) {
    std::fprintf(stderr, "Error: 无法确定项目根目录。\n"
                         "请在 config.json 中设置 ProjectsDir。\n");
    std::getchar();
    return;
  }

  std::printf("  项目根目录:  %s\n", projects_dir.c_str());

  // ---- 4. 构建配置 ----
  KMeansConfig config = build_config(cfg, projects_dir);

  // 渲染采样上限
  u64 render_max_points =
    static_cast<u64>(cfg.get_int("output.render_max_points", 100000));

  // ---- 5. 打印配置 ----
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
  std::printf("  渲染采样:    %lu 点\n", render_max_points);
  std::printf("\n");

  // ---- 6. 运行 K-Means ----
  KMeans kmeans(config);
  auto result = kmeans.run();

  // ---- 7. 输出结果 ----
  print_result(result);

  // 获取数据点数
  u64 num_points = 0;
  {
    Dataset tmp;
    if (tmp.load(config.data_path, true)) {
      num_points = tmp.size();
    }
  }

  // UTC+8 时间戳
  std::string ts = get_timestamp_utc8();

  // 输出目录: results/YYYYMMDD_HHMMSS/
  std::string output_dir = projects_dir + "/results/" + ts;
  ensure_dir(output_dir);

  // 固定文件名
  save_result_txt(output_dir, config, result, ts);
  save_labels_bin(output_dir, result, num_points);
  save_centers_csv(output_dir, result);
  save_render_csv(output_dir, config, result, num_points, render_max_points);

  std::printf("\n运行完毕！按 Enter 键退出...\n");
  std::getchar();
}

// ============================================================
// 主函数
// ============================================================
int main() {
  std::setbuf(stdout, nullptr);
  kmeans_run();
  return 0;
}
