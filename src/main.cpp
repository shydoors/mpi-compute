/**
 * @file main.cpp
 * @brief K-Means 聚类 — 主入口（读 config.json，VS2019 直接运行版）
 *
 * 使用方法（VS2019）:
 *   1. 修改 src/config.json 中的运行参数
 *   2. 按 F5（调试运行）或 Ctrl+F5（直接运行）
 *    无需重新编译，改完 config.json 直接点运行即可。
 *
 * 输出文件（自动保存到 results/ 目录）:
 *   result_YYYYMMDD_HHMMSS.txt     — 运行日志
 *   labels_YYYYMMDD_HHMMSS.bin     — 所有点的聚类标签（二进制，不丢失数据）
 *   centers_YYYYMMDD_HHMMSS.csv    — 各簇中心坐标
 *   render_YYYYMMDD_HHMMSS.csv     — 渲染用采样点（超大量时自动均匀采样）
 */

#include "kmeans.hpp"
#include "types.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

// ============================================================
// JSON 解析器（极简，仅支持本项目需要的 JSON 子集）
// ============================================================
namespace {

// 跳过空白字符
static void skip_ws(const std::string& s, size_t& pos) {
  while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' ||
         s[pos] == '\n' || s[pos] == '\r'))
    ++pos;
}

// 读取一个 JSON 字符串（带引号）
static std::string parse_json_string(const std::string& s, size_t& pos) {
  skip_ws(s, pos);
  if (pos >= s.size() || s[pos] != '"') return "";
  ++pos; // 跳过开头的 "
  std::string result;
  while (pos < s.size() && s[pos] != '"') {
    if (s[pos] == '\\' && pos + 1 < s.size()) {
      ++pos;
      if (s[pos] == 'n') result += '\n';
      else if (s[pos] == 't') result += '\t';
      else result += s[pos];
    } else {
      result += s[pos];
    }
    ++pos;
  }
  if (pos < s.size()) ++pos; // 跳过结尾的 "
  return result;
}

// 读取一个 JSON 值（number / true / false / null）
static std::string parse_json_value(const std::string& s, size_t& pos) {
  skip_ws(s, pos);
  size_t start = pos;
  if (pos < s.size() && s[pos] == '"') {
    return parse_json_string(s, pos);
  }
  while (pos < s.size() && s[pos] != ',' && s[pos] != '}' &&
         s[pos] != ']' && s[pos] != ' ' && s[pos] != '\t' &&
         s[pos] != '\n' && s[pos] != '\r') {
    ++pos;
  }
  return s.substr(start, pos - start);
}

// 解析 JSON 对象中的顶层键值对（只读我们关心的字段）
class SimpleJsonReader {
public:
  explicit SimpleJsonReader(const std::string& path) {
    std::ifstream ifs(path);
    if (ifs) {
      content_ = std::string((std::istreambuf_iterator<char>(ifs)),
                              std::istreambuf_iterator<char>());
    }
  }

  bool valid() const { return !content_.empty(); }

  // 读取字符串值：先找到 key，再读其后的 value
  std::string get_string(const std::string& key) const {
    size_t pos = content_.find('"' + key + '"');
    if (pos == std::string::npos) return "";
    pos = content_.find(':', pos + key.size() + 2);
    if (pos == std::string::npos) return "";
    ++pos;
    return parse_json_value(content_, pos);
  }

  int get_int(const std::string& key) const {
    auto s = get_string(key);
    if (s.empty()) return 0;
    return std::stoi(s);
  }

  double get_double(const std::string& key) const {
    auto s = get_string(key);
    if (s.empty()) return 0.0;
    return std::stod(s);
  }

  bool get_bool(const std::string& key) const {
    auto s = get_string(key);
    if (s == "true") return true;
    if (s == "false") return false;
    return std::stoi(s) != 0;
  }

  // 读取嵌套对象中的字段：如 "output" -> "render_max_points"
  std::string get_nested_string(const std::string& obj_key,
                                const std::string& field_key) const {
    size_t pos = content_.find('"' + obj_key + '"');
    if (pos == std::string::npos) return "";
    pos = content_.find('"' + field_key + '"', pos + obj_key.size() + 2);
    if (pos == std::string::npos) return "";
    pos = content_.find(':', pos + field_key.size() + 2);
    if (pos == std::string::npos) return "";
    ++pos;
    return parse_json_value(content_, pos);
  }

  int get_nested_int(const std::string& obj_key,
                     const std::string& field_key) const {
    auto s = get_nested_string(obj_key, field_key);
    if (s.empty()) return 0;
    return std::stoi(s);
  }

private:
  std::string content_;
};

} // anonymous namespace

// ============================================================
// 输出目录
// ============================================================
static const char* kOutputDir = "results";

static void ensure_output_dir() {
#ifdef _WIN32
  std::system(("if not exist " + std::string(kOutputDir) + " mkdir " + std::string(kOutputDir)).c_str());
#else
  std::system(("mkdir -p " + std::string(kOutputDir)).c_str());
#endif
}

// ============================================================
// 保存日志到文件
// ============================================================
static void save_log(const KMeansConfig& config,
                     const KMeansResult& result,
                     const char* timestamp) {
  char path[256];
  std::snprintf(path, sizeof(path), "%s/result_%s.txt", kOutputDir, timestamp);

  std::FILE* fp = std::fopen(path, "w");
  if (!fp) { std::fprintf(stderr, "Error: 无法写入 %s\n", path); return; }

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
  std::fprintf(fp, "\n");
  std::fprintf(fp, "--- 运行结果 ---\n");
  std::fprintf(fp, "  簇数 (K):          %u\n", result.k);
  std::fprintf(fp, "  迭代次数:          %d\n", result.iterations);
  std::fprintf(fp, "  SSE (Inertia):     %.6e\n", result.inertia);
  std::fprintf(fp, "  加载时间:          %.3f s\n", result.time_load);
  std::fprintf(fp, "  自动 K 推导时间:   %.3f s\n", result.time_auto_k);
  std::fprintf(fp, "  迭代时间:          %.3f s\n", result.time_iterate);
  std::fprintf(fp, "  总运行时间:        %.3f s\n", result.time_total);
  std::fprintf(fp, "========================================\n");
  std::fclose(fp);
  std::printf("  日志已保存至: %s\n", path);
}

// ============================================================
// 保存所有点的聚类标签（二进制，u32 数组）
// ============================================================
static void save_labels_bin(const KMeansResult& result,
                            u64 num_points,
                            const char* timestamp) {
  if (!result.labels || num_points == 0) return;

  char path[256];
  std::snprintf(path, sizeof(path), "%s/labels_%s.bin", kOutputDir, timestamp);

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

// ============================================================
// 导出中心点（CSV）
// ============================================================
static void save_centers_csv(const KMeansResult& result,
                             const char* timestamp) {
  if (!result.centers_x || !result.centers_y || result.k == 0) return;

  char path[256];
  std::snprintf(path, sizeof(path), "%s/centers_%s.csv", kOutputDir, timestamp);

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

// ============================================================
// 导出渲染用采样点（CSV）
// ============================================================
static void save_render_csv(const KMeansConfig& config,
                            const KMeansResult& result,
                            u64 num_points,
                            u64 max_render_points,
                            const char* timestamp) {
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

  char path[256];
  std::snprintf(path, sizeof(path), "%s/render_%s.csv", kOutputDir, timestamp);

  std::FILE* fp = std::fopen(path, "w");
  if (!fp) { std::fprintf(stderr, "Error: 无法写入 %s\n", path); return; }

  std::fprintf(fp, "x,y,label\n");

  const f64* xs = data.x();
  const f64* ys = data.y();

  if (num_points <= max_render_points) {
    for (u64 i = 0; i < num_points; ++i) {
      std::fprintf(fp, "%.6f,%.6f,%u\n", xs[i], ys[i], result.labels[i]);
    }
    std::printf("  渲染数据已保存至: %s  (%lu 个点, 全量)\n", path, num_points);
  } else {
    u64 step = num_points / max_render_points;
    u64 actual = 0;
    for (u64 i = 0; i < num_points; i += step) {
      std::fprintf(fp, "%.6f,%.6f,%u\n", xs[i], ys[i], result.labels[i]);
      ++actual;
    }
    std::printf("  渲染数据已保存至: %s  (%lu 个点, 采样步长 %lu, 约 %lu 个点)\n",
                path, num_points, step, actual);
  }

  std::fclose(fp);
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
// 主入口 — 读 config.json → 运行 K-Means → 输出结果
// ============================================================
static void kmeans_run() {
  // ---- 1. 读取配置文件 ----
  SimpleJsonReader cfg("src/config.json");
  if (!cfg.valid()) {
    std::fprintf(stderr, "Error: 无法读取 src/config.json\n");
    std::fprintf(stderr, "请确保该文件存在且格式正确。\n");
    std::getchar();
    return;
  }

  // ---- 2. 构建配置 ----
  KMeansConfig config;
  config.data_path = cfg.get_string("data_path");
  if (config.data_path.empty()) {
    config.data_path = "data/a.dat";
  }

  std::string backend_str = cfg.get_string("backend");
  // config.json 中 backend 是对象，需要读 "value"
  if (backend_str.empty() || backend_str.front() == '{') {
    // 尝试从嵌套对象中读 "value"
    backend_str = cfg.get_nested_string("backend", "value");
  }
  if (backend_str == "seq")       config.backend = Backend::Sequential;
  else if (backend_str == "omp")  config.backend = Backend::OpenMP;
  else if (backend_str == "cuda") config.backend = Backend::CUDA;
  else {
    std::fprintf(stderr, "Warning: 未知后端 '%s'，使用默认 omp\n",
                 backend_str.c_str());
    config.backend = Backend::OpenMP;
  }

  config.auto_k         = cfg.get_bool("auto_k");
  config.fixed_k        = static_cast<u32>(cfg.get_int("fixed_k"));
  config.max_iterations = cfg.get_int("max_iterations");
  config.threshold      = cfg.get_double("threshold");
  config.streaming      = cfg.get_bool("streaming");
  config.checkpoint_path = cfg.get_string("checkpoint_path");
  config.resume         = cfg.get_bool("resume");
  config.output_path    = "";

  // 渲染采样上限（从 output 嵌套对象读）
  u64 render_max_points = static_cast<u64>(
      cfg.get_nested_int("output", "render_max_points"));
  if (render_max_points == 0) render_max_points = 100000;

  // ---- 3. 打印配置 ----
  std::printf("========================================\n");
  std::printf("  K-Means Clustering\n");
  std::printf("  配置文件:      src/config.json\n");
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

  // ---- 4. 运行 K-Means ----
  KMeans kmeans(config);
  auto result = kmeans.run();

  // ---- 5. 输出结果 ----
  print_result(result);

  // 获取数据点数（重新加载文件头即可）
  u64 num_points = 0;
  {
    Dataset tmp;
    if (tmp.load(config.data_path, true)) {
      num_points = tmp.size();
    }
  }

  // 时间戳
  auto now  = std::chrono::system_clock::now();
  auto tt   = std::chrono::system_clock::to_time_t(now);
  char timestamp[64];
  std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S",
                std::localtime(&tt));

  // 保存
  ensure_output_dir();
  save_log(config, result, timestamp);
  save_labels_bin(result, num_points, timestamp);
  save_centers_csv(result, timestamp);
  save_render_csv(config, result, num_points, render_max_points, timestamp);

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
