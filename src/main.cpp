/**
 * @file main.cpp
 * @brief K-Means 聚类 — 主入口（读 config.json，VS2019 直接运行版）
 *
 * 使用方法:
 *   1. 修改 src/config.json 中的运行参数
 *   2. 编译运行：./build/kmeans（或 VS2019 按 F5）
 *    改 config.json 无需重新编译。
 *
 * 路径解析规则:
 *   程序启动时自动检测项目根目录（从 CWD 向上查找含 data/ 和 src/ 的目录）。
 *   config.json 中的所有相对路径字段（data_path, checkpoint_path 等）
 *   都会基于 ProjectsDir 拼接为绝对路径后再传给 K-Means 核心。
 *
 *   ProjectsDir 也可以在 config.json 中手动指定（绝对路径），
 *   此时跳过自动检测。
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
// 极简 JSON 解析
// ============================================================
namespace {

static std::string load_file(const std::string& path) {
  std::ifstream ifs(path);
  if (!ifs) return "";
  return std::string((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());
}

static std::string trim(const std::string& s) {
  size_t a = 0, b = s.size();
  while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\n' || s[a] == '\r')) ++a;
  while (b > a && (s[b-1] == ' ' || s[b-1] == '\t' || s[b-1] == '\n' || s[b-1] == '\r')) --b;
  return s.substr(a, b - a);
}

static std::string unquote(const std::string& s) {
  auto t = trim(s);
  if (t.size() >= 2 && t.front() == '"' && t.back() == '"')
    return t.substr(1, t.size() - 2);
  return t;
}

static std::string find_value_raw(const std::string& content, const std::string& key) {
  std::string pattern = '"' + key + '"';
  size_t pos = content.find(pattern);
  if (pos == std::string::npos) return "";

  pos = content.find(':', pos + pattern.size());
  if (pos == std::string::npos) return "";

  ++pos;
  while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t' ||
         content[pos] == '\n' || content[pos] == '\r')) ++pos;
  if (pos >= content.size()) return "";

  if (content[pos] == '"') {
    size_t end = pos + 1;
    while (end < content.size()) {
      if (content[end] == '"' && (end == pos + 1 || content[end-1] != '\\')) break;
      ++end;
    }
    if (end < content.size()) ++end;
    return content.substr(pos, end - pos);
  } else if (content[pos] == '{' || content[pos] == '[') {
    char open = content[pos];
    char close = (open == '{') ? '}' : ']';
    int depth = 1;
    size_t end = pos + 1;
    while (end < content.size() && depth > 0) {
      if (content[end] == open) ++depth;
      else if (content[end] == close) --depth;
      ++end;
    }
    return content.substr(pos, end - pos);
  } else {
    size_t end = pos;
    while (end < content.size() && content[end] != ',' && content[end] != '}' &&
           content[end] != ']' && content[end] != ' ' && content[end] != '\t' &&
           content[end] != '\n' && content[end] != '\r') ++end;
    return content.substr(pos, end - pos);
  }
}

static std::string read_string(const std::string& content, const std::string& key) {
  return unquote(find_value_raw(content, key));
}

static std::string read_nested_string(const std::string& content,
                                      const std::string& obj_key,
                                      const std::string& field_key) {
  std::string obj = find_value_raw(content, obj_key);
  if (obj.empty()) return "";
  return unquote(find_value_raw(obj, field_key));
}

static int read_int(const std::string& content, const std::string& key) {
  auto s = trim(find_value_raw(content, key));
  if (s.empty()) return 0;
  return std::stoi(s);
}

static double read_double(const std::string& content, const std::string& key) {
  auto s = trim(find_value_raw(content, key));
  if (s.empty()) return 0.0;
  return std::stod(s);
}

static bool read_bool(const std::string& content, const std::string& key) {
  auto s = trim(find_value_raw(content, key));
  if (s == "true") return true;
  if (s == "false") return false;
  return std::stoi(s) != 0;
}

} // anonymous namespace

// ============================================================
// 路径工具
// ============================================================
namespace {

/// 判断路径是否为绝对路径
static bool is_absolute_path(const std::string& path) {
  if (path.empty()) return false;
  if (path[0] == '/') return true;
#ifdef _WIN32
  if (path.size() >= 2 && std::isalpha(path[0]) && path[1] == ':') return true;
#endif
  return false;
}

/// 拼接两个路径（处理中间的 /）
static std::string join_path(const std::string& base, const std::string& rel) {
  if (rel.empty()) return base;
  if (is_absolute_path(rel)) return rel;
  if (base.empty()) return rel;

  std::string result = base;
  if (result.back() != '/') result += '/';
  result += rel;
  return result;
}

/// 如果 path 是相对路径，则基于 base 拼接为绝对路径
static std::string resolve_path(const std::string& path, const std::string& base) {
  if (path.empty()) return path;
  if (is_absolute_path(path)) return path;
  return join_path(base, path);
}

/// 自动检测项目根目录：从 CWD 向上查找，直到发现 data/ 和 src/ 同时存在
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

/// 确保目录存在
static void ensure_dir(const std::string& dir) {
#ifdef _WIN32
  std::system(("if not exist \"" + dir + "\" mkdir \"" + dir + "\"").c_str());
#else
  std::system(("mkdir -p \"" + dir + "\"").c_str());
#endif
}

} // anonymous namespace

// ============================================================
// 保存日志到文件
// ============================================================
static void save_log(const std::string& output_dir,
                     const KMeansConfig& config,
                     const KMeansResult& result,
                     const char* timestamp) {
  char path[512];
  std::snprintf(path, sizeof(path), "%s/result_%s.txt", output_dir.c_str(), timestamp);

  std::FILE* fp = std::fopen(path, "w");
  if (!fp) { std::fprintf(stderr, "Error: 无法写入 %s\n", path); return; }

  std::fprintf(fp, "========================================\n");
  std::fprintf(fp, "  K-Means 聚类运行结果\n");
  std::fprintf(fp, "  运行时间: %s\n", timestamp);
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
static void save_labels_bin(const std::string& output_dir,
                            const KMeansResult& result,
                            u64 num_points,
                            const char* timestamp) {
  if (!result.labels || num_points == 0) return;

  char path[512];
  std::snprintf(path, sizeof(path), "%s/labels_%s.bin", output_dir.c_str(), timestamp);

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
static void save_centers_csv(const std::string& output_dir,
                             const KMeansResult& result,
                             const char* timestamp) {
  if (!result.centers_x || !result.centers_y || result.k == 0) return;

  char path[512];
  std::snprintf(path, sizeof(path), "%s/centers_%s.csv", output_dir.c_str(), timestamp);

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
static void save_render_csv(const std::string& output_dir,
                            const KMeansConfig& config,
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

  char path[512];
  std::snprintf(path, sizeof(path), "%s/render_%s.csv", output_dir.c_str(), timestamp);

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
// 主入口
// ============================================================
static void kmeans_run() {
  // ---- 1. 确定项目根目录 ----
  // 先自动检测（从 CWD 向上找含 data/ 和 src/ 的目录）
  std::string projects_dir = detect_project_dir();

  // ---- 2. 定位并读取 config.json ----
  std::string config_content;
  if (!projects_dir.empty()) {
    config_content = load_file(projects_dir + "/src/config.json");
  }
  if (config_content.empty()) {
    // 自动检测失败了，试试从 CWD 读
    config_content = load_file("src/config.json");
  }
  if (config_content.empty()) {
    // 试试 config.json 里的 ProjectsDir（需要先想办法读到 config.json...）
    // 到这一步说明确实找不到了，报错
    std::fprintf(stderr, "Error: 找不到 src/config.json\n"
                         "请确保在项目根目录下运行。\n"
                         "或在 config.json 中设置 ProjectsDir 为项目根目录的绝对路径。\n");
    std::getchar();
    return;
  }

  // ---- 3. 从 config.json 中读 ProjectsDir（如果有，覆盖自动检测的结果） ----
  {
    std::string cfg_dir = read_string(config_content, "ProjectsDir");
    if (!cfg_dir.empty()) {
      // 如果配置里的 ProjectsDir 是相对路径，基于 CWD 拼接
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
                         "请在 src/config.json 中设置 ProjectsDir。\n");
    std::getchar();
    return;
  }

  std::printf("  项目根目录:  %s\n", projects_dir.c_str());

  // ---- 4. 构建配置 ---- // ---- 3. 构建配置（先解析相对路径，再拼接为绝对路径） ----
  KMeansConfig config;
  config.projects_dir = projects_dir;

  // 数据路径
  std::string raw_data_path = read_string(config_content, "data_path");
  if (raw_data_path.empty()) raw_data_path = "data/a.dat";
  config.data_path = resolve_path(raw_data_path, projects_dir);

  // 后端
  std::string backend_str = read_nested_string(config_content, "backend", "value");
  if (backend_str.empty()) backend_str = read_string(config_content, "backend");
  if (backend_str == "seq")       config.backend = Backend::Sequential;
  else if (backend_str == "omp")  config.backend = Backend::OpenMP;
  else if (backend_str == "cuda") config.backend = Backend::CUDA;
  else {
    std::fprintf(stderr, "Warning: 未知后端 '%s'，使用默认 omp\n",
                 backend_str.c_str());
    config.backend = Backend::OpenMP;
  }

  config.auto_k         = read_bool(config_content, "auto_k");
  config.fixed_k        = static_cast<u32>(read_int(config_content, "fixed_k"));
  config.max_iterations = read_int(config_content, "max_iterations");
  config.threshold      = read_double(config_content, "threshold");
  config.streaming      = read_bool(config_content, "streaming");

  // checkpoint 路径（同样基于 ProjectsDir 拼接）
  config.checkpoint_path = resolve_path(
      read_string(config_content, "checkpoint_path"), projects_dir);
  config.resume   = read_bool(config_content, "resume");
  config.output_path = "";

  // 渲染采样上限
  u64 render_max_points = 100000;
  {
    auto obj = find_value_raw(config_content, "output");
    if (!obj.empty()) {
      auto v = read_int(obj, "render_max_points");
      if (v > 0) render_max_points = static_cast<u64>(v);
    }
  }

  // 输出目录
  std::string output_dir = projects_dir + "/results";

  // ---- 5. 打印配置 ----
  std::printf("========================================\n");
  std::printf("  K-Means Clustering\n");
  std::printf("========================================\n\n");

  std::printf("  数据文件:    %s\n", config.data_path.c_str());
  std::printf("  输出目录:    %s\n", output_dir.c_str());
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

  u64 num_points = 0;
  {
    Dataset tmp;
    if (tmp.load(config.data_path, true)) {
      num_points = tmp.size();
    }
  }

  auto now  = std::chrono::system_clock::now();
  auto tt   = std::chrono::system_clock::to_time_t(now);
  char timestamp[64];
  std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S",
                std::localtime(&tt));

  ensure_dir(output_dir);
  save_log(output_dir, config, result, timestamp);
  save_labels_bin(output_dir, result, num_points, timestamp);
  save_centers_csv(output_dir, result, timestamp);
  save_render_csv(output_dir, config, result, num_points,
                  render_max_points, timestamp);

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
