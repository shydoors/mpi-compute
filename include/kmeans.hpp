#ifndef KMEANS_HPP
#define KMEANS_HPP

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
  std::string data_path;
  Backend     backend{Backend::CUDA};
  bool        use_mmap{true};

  // K 推导
  bool        auto_k{true};
  u32         fixed_k{0};
  u64         sample_size{kDefaultSampleSize};

  // 迭代
  i32         max_iterations{kMaxIterations};
  f64         threshold{kConvergenceThreshold};

  // 流式
  bool        streaming{false};
  u64         stream_chunk_size{64ULL * 1024 * 1024};

  // 断点继续
  std::string checkpoint_path;
  i32         checkpoint_interval{5};
  bool        resume{false};

  // 项目根目录（用于日志/路径显示）
  std::string projects_dir;

  // 输出
  std::string output_path;
};

// ============================================================
// K-Means 结果
// ============================================================
struct KMeansResult {
  u32  k{0};
  i32  iterations{0};
  f64  inertia{0.0};

  std::unique_ptr<f64[]> centers_x;
  std::unique_ptr<f64[]> centers_y;

  std::unique_ptr<u32[]> labels;

  f64  time_load{0.0};
  f64  time_autok{0.0};
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

  [[nodiscard]] KMeansResult run();

  [[nodiscard]] u32 auto_detect_k(const Dataset& sample) const;

  [[nodiscard]] bool save_checkpoint(i32 iteration,
                                     const f64* cx, const f64* cy, u32 k);
  [[nodiscard]] bool load_checkpoint(f64* cx, f64* cy, u32& k, i32& iteration);

private:
  KMeansResult run_sequential(Dataset& data, u32 k);
  KMeansResult run_omp(Dataset& data, u32 k);
  KMeansResult run_cuda(Dataset& data, u32 k);

private:
  KMeansConfig config_;
};

// ============================================================
// 自由函数声明
// ============================================================

/// CPU 串行 K-Means（支持 SoA 和交错模式）
/// 当 interleaved=true 时，xs 是交错数据 (x,y,x,y,...)，ys 传 nullptr
[[nodiscard]] KMeansResult run_kmeans_sequential(
    const f64* xs, const f64* ys, u64 n, u32 k,
    i32 max_iter, f64 threshold,
    f64* init_cx, f64* init_cy,
    bool interleaved,
    u32* labels_out);

/// OpenMP 并行 K-Means（支持 SoA 和交错模式）
[[nodiscard]] KMeansResult run_kmeans_omp(
    const f64* xs, const f64* ys, u64 n, u32 k,
    i32 max_iter, f64 threshold,
    f64* init_cx, f64* init_cy,
    bool interleaved,
    u32* labels_out);

#endif  // KMEANS_HPP
