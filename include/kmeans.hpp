#ifndef KMEANS_HPP
#define KMEANS_HPP

/**
 * @file kmeans.hpp
 * @brief K-Means 算法主接口
 *
 * 支持三种并行后端：
 *   1. Sequential （CPU 串行，用于验证）
 *   2. OpenMP     （CPU 多核）
 *   3. CUDA       （GPU 加速）
 *
 * 自动 K 推导：采样 + 肘部检测（Kneedle 算法）
 * 断点继续：支持 checkpoint 序列化/反序列化
 */

#include "types.hpp"
#include "dataset.hpp"
#include "checkpoint.hpp"

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

// ============================================================
// 运行模式
// ============================================================
enum class Backend : i32 {
  Sequential = 0,
  OpenMP     = 1,
  CUDA       = 2
};

inline const char* backend_name(Backend b) noexcept {
  switch (b) {
    case Backend::Sequential: return "Sequential";
    case Backend::OpenMP:     return "OpenMP";
    case Backend::CUDA:       return "CUDA";
  }
  return "Unknown";
}

// ============================================================
// 配置参数
// ============================================================
struct KMeansConfig {
  // 数据
  std::string data_path;       // 数据文件（相对路径）
  Backend     backend{Backend::CUDA};
  bool        use_mmap{true};  // 是否使用 mmap 加载

  // K 推导
  bool        auto_k{true};    // 是否自动推导簇数
  u32         fixed_k{0};      // 手动指定 K（auto_k=false 时生效）
  u64         sample_size{kDefaultSampleSize};

  // 迭代
  i32         max_iterations{kMaxIterations};
  f64         threshold{kConvergenceThreshold};

  // 流式
  bool        streaming{false};         // 是否使用流式读取（内存 < 16GiB 时）
  u64         stream_chunk_size{64ULL * 1024 * 1024};

  // 断点继续
  std::string checkpoint_path;          // checkpoint 文件路径（空=不使用）
  i32         checkpoint_interval{5};   // 每 N 轮写一次 checkpoint
  bool        resume{false};            // 是否从 checkpoint 恢复

  // 输出
  std::string output_path;              // 聚类结果输出路径（空=不输出）
};

// ============================================================
// K-Means 结果
// ============================================================
struct KMeansResult {
  u32  k{0};                    // 最终簇数
  i32  iterations{0};           // 实际迭代次数
  f64  inertia{0.0};            // 最终 SSE

  // 中心点（CPU 端，长度 k）
  std::unique_ptr<f64[]> centers_x;
  std::unique_ptr<f64[]> centers_y;

  // 每个点的簇标签（长度 = 数据集点数）
  // 由 Dataset 所有，此处只引用
  std::unique_ptr<u32[]> labels;

  // 运行时间（秒）
  f64  time_load{0.0};
  f64  time_auto_k{0.0};
  f64  time_iterate{0.0};
  f64  time_total{0.0};
};

// ============================================================
// K-Means 主类
// ============================================================
class KMeans {
public:
  explicit KMeans(const KMeansConfig& config);
  ~KMeans();

  KMeans(const KMeans&) = delete;
  KMeans& operator=(const KMeans&) = delete;

  /// 运行完整流程：加载 → 自动 K → 迭代 → 输出
  [[nodiscard]] KMeansResult run();

  /// 仅执行自动 K 推导
  [[nodiscard]] u32 auto_detect_k(const Dataset& sample) const;

  /// 保存 checkpoint
  [[nodiscard]] bool save_checkpoint(i32 iteration,
                                     const f64* cx, const f64* cy, u32 k);

  /// 加载 checkpoint
  [[nodiscard]] bool load_checkpoint(f64* cx, f64* cy, u32& k, i32& iteration);

private:
  // 后端调度
  KMeansResult run_sequential(Dataset& data, u32 k);
  KMeansResult run_omp(Dataset& data, u32 k);
  KMeansResult run_cuda(Dataset& data, u32 k);

private:
  KMeansConfig config_;
};

// ============================================================
// 自由函数声明（供各后端实现使用）
// ============================================================

/// CPU 串行 K-Means 实现
[[nodiscard]] KMeansResult run_kmeans_sequential(
    const f64* xs, const f64* ys, u64 n, u32 k,
    i32 max_iter, f64 threshold,
    f64* init_cx, f64* init_cy,
    const std::string& checkpoint_path,
    i32 checkpoint_interval,
    bool resume,
    u32* labels_out);

/// OpenMP 并行 K-Means 实现
[[nodiscard]] KMeansResult run_kmeans_omp(
    const f64* xs, const f64* ys, u64 n, u32 k,
    i32 max_iter, f64 threshold,
    f64* init_cx, f64* init_cy,
    const std::string& checkpoint_path,
    i32 checkpoint_interval,
    bool resume,
    u32* labels_out);

#endif  // KMEANS_HPP

