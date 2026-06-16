/**
 * @file main.cpp
 * @brief K-Means 聚类 -- 编排层
 *
 * main.cpp 只做两件事：
 *   1. 解析 config.json（通过 JsonConfig）
 *   2. 把配置丢给 KMeans 跑
 *
 * 其他所有逻辑（路径、输出、checkpoint）已在各自模块中。
 *
 * checkpoint 由 KMeans 内部调用 kmeans/checkpoint.cpp，
 * main.cpp 不接触 checkpoint 逻辑。
 */

#include "json_config.hpp"
#include "kmeans.hpp"
#include "output_writer.hpp"
#include "path_util.hpp"
#include "types.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

// ============================================================
// 配置构建
// ============================================================

static KMeansConfig build_config(const JsonConfig& cfg,
                                  const std::string& projects_dir)
{
  KMeansConfig config{};
  config.projects_dir = projects_dir;

  // data_path
  {
    std::string raw = cfg.get_string("data_path.value");
    if (raw.empty()) { raw = cfg.get_string("data_path"); }
    if (raw.empty()) { raw = "data/a.dat"; }
    config.data_path = path_resolve(raw, projects_dir);
  }

  // backend
  {
    std::string bs = cfg.get_string("backend.value");
    if (bs.empty()) { bs = cfg.get_string("backend"); }
    if (bs == "seq") {
      config.backend = Backend::Sequential;
    } else if (bs == "omp") {
      config.backend = Backend::OpenMP;
    } else if (bs == "cuda") {
      config.backend = Backend::CUDA;
    } else {
      std::fprintf(stderr,
                   "Warning: 未知后端 '%s'，使用默认 cuda\n",
                   bs.c_str());
      config.backend = Backend::CUDA;
    }
  }

  // K 值
  {
    const i32 k = cfg.get_int("k.value", 0);
    if (k <= 0) {
      config.auto_k  = true;
      config.fixed_k = 0;
    } else {
      config.auto_k  = false;
      config.fixed_k = static_cast<u32>(k);
    }
  }

  // 迭代参数
  config.max_iterations = cfg.get_int("max_iterations", kMaxIterations);
  config.threshold      = cfg.get_double("threshold", kConvergenceThreshold);

  // 流式
  config.streaming = cfg.get_bool("streaming", false);

  // 断点
  {
    const std::string cp = cfg.get_string("checkpoint_path");
    config.checkpoint_path = path_resolve(cp, projects_dir);
  }
  config.resume = cfg.get_bool("resume", false);

  // 输出
  config.output_path = {};

  return config;
}

// ============================================================
// 主入口
// ============================================================

static void kmeans_run()
{
  // 1. 确定项目根目录
  std::string projects_dir = detect_project_dir();

  // 2. 定位并读取 config.json
  JsonConfig cfg{};
  bool loaded = false;
  if (!projects_dir.empty()) {
    loaded = cfg.load(projects_dir + "/config.json");
  }
  if (!loaded) {
    loaded = cfg.load("config.json");
  }
  if (!loaded) {
    std::fprintf(stderr,
                 "Error: 找不到 config.json\n"
                 "请确保在项目根目录下运行。\n"
                 "或在 config.json 中设置 projects_dir 为项目根目录的绝对路径。\n");
    std::getchar();
    return;
  }

  // 3. 从 config.json 中读 ProjectsDir（可覆盖自动检测结果）
  {
    std::string cfg_dir = cfg.get_string("ProjectsDir");
    if (!cfg_dir.empty()) {
      if (!path_is_absolute(cfg_dir)) {
        char buf[PATH_MAX] = {};
        if (::getcwd(buf, sizeof(buf)) != nullptr) {
          cfg_dir = path_join(std::string(buf), cfg_dir);
        }
      }
      projects_dir = cfg_dir;
    }
  }

  if (projects_dir.empty()) {
    std::fprintf(stderr,
                 "Error: 无法确定项目根目录。\n"
                 "请在 config.json 中设置 ProjectsDir。\n");
    std::getchar();
    return;
  }

  std::printf("  项目根目录:  %s\n", projects_dir.c_str());

  // 4. 构建配置
  KMeansConfig config = build_config(cfg, projects_dir);
  const u64 render_max_points =
    static_cast<u64>(cfg.get_int("output.render_max_points", 100000));

  // 5. 打印配置
  output_print_config(config, render_max_points);

  // 6. 运行 K-Means
  KMeans kmeans{config};
  auto result = kmeans.run();

  // 7. 控制台输出结果
  output_print_result(result);

  // 8. 获取数据点数（用于输出文件）
  u64 num_points = 0;
  {
    Dataset tmp{};
    if (tmp.load(config.data_path, true)) {
      num_points = tmp.size();
    }
  }

  // 9. 文件输出
  const std::string ts = timestamp_utc8();
  const std::string output_dir = projects_dir + "/results/" + ts;
  ensure_dir(output_dir);

  output_save_result_txt(output_dir.c_str(), config, result, ts.c_str());
  output_save_labels_bin(output_dir.c_str(), result, num_points);
  output_save_centers_csv(output_dir.c_str(), result);
  output_save_render_csv(output_dir.c_str(), config, result,
                          num_points, render_max_points);

  std::printf("\n运行完毕！按 Enter 键退出...\n");
  std::getchar();
}

// ============================================================
// 主函数
// ============================================================

int main()
{
  std::setbuf(stdout, nullptr);
  kmeans_run();
  return 0;
}
