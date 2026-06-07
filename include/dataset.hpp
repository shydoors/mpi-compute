#ifndef DATASET_HPP
#define DATASET_HPP

/**
 * @file dataset.hpp
 * @brief 数据集管理 — 内存映射 / 流式读取 / 采样
 *
 * 设计目标：
 *   - 支持 9+ GiB 超大数据集的 mmap 或流式分块读取
 *   - 提供均匀采样接口，用于自动 K 推导
 *   - SoA 布局（分离 x/y 数组），便于 SIMD / GPU 批量访问
 *
 * 加载策略：
 *   - 小文件（< 1 GiB）：mmap + SoA 转换（x/y 分离），兼容所有后端
 *   - 大文件（>= 1 GiB）：mmap 零拷贝，保留交错数据，K-Means 直接访问
 *     x() / y() 在大文件模式下不可用，使用 raw_data() 和 raw_x(i)/raw_y(i)
 */

#include "types.hpp"

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// ============================================================
// 数据集类
// ============================================================
class Dataset {
public:
  /// 加载模式
  enum class Mode : i32 {
    None        = 0,   ///< 未加载
    SoA         = 1,   ///< SoA 分离（小文件，兼容所有后端）
    Interleaved = 2,   ///< mmap 交错零拷贝（大文件，仅 CPU 后端可访问）
  };

  Dataset() = default;

  Dataset(const Dataset&) = delete;
  Dataset& operator=(const Dataset&) = delete;

  Dataset(Dataset&&) noexcept = delete;
  Dataset& operator=(Dataset&&) noexcept = delete;

  ~Dataset();

  // ----------------------------------------------------------
  // 加载方式
  // ----------------------------------------------------------

  /// 全量加载
  [[nodiscard]] bool load(const std::string& path, bool use_mmap = true);

  /// 流式分块读取
  [[nodiscard]] bool load_streaming(const std::string& path,
                                    u64 chunk_size = 64ULL * 1024 * 1024);

  /// 从已有内存指针恢复（断点继续用）
  [[nodiscard]] bool from_memory(f64* x_data, f64* y_data, u64 num_points);

  // ----------------------------------------------------------
  // 采样
  // ----------------------------------------------------------

  [[nodiscard]] std::vector<f64> uniform_sample(u64 sample_size = kDefaultSampleSize) const;

  // ----------------------------------------------------------
  // 访问器
  // ----------------------------------------------------------

  [[nodiscard]] inline u64  size()   const noexcept { return num_points_; }
  [[nodiscard]] inline u64  bytes()  const noexcept { return num_points_ * kBytesPerPoint; }
  [[nodiscard]] inline bool empty()  const noexcept { return num_points_ == 0; }
  [[nodiscard]] inline Mode mode()   const noexcept { return mode_; }

  /// SoA 模式：返回 x/y 数组指针（仅 SoA 模式可用）
  [[nodiscard]] inline const f64* x() const noexcept { return x_.get(); }
  [[nodiscard]] inline const f64* y() const noexcept { return y_.get(); }
  [[nodiscard]] inline f64*       x_mut() noexcept { return x_.get(); }
  [[nodiscard]] inline f64*       y_mut() noexcept { return y_.get(); }

  /// Interleaved 模式：返回原始交错数据指针（每个点 2 个 f64: x, y, x, y, ...）
  [[nodiscard]] inline const f64* raw_data() const noexcept {
    return static_cast<const f64*>(mmap_addr_);
  }

  /// Interleaved 模式：获取第 i 个点的 x 坐标
  [[nodiscard]] inline f64 raw_x(u64 i) const noexcept {
    return static_cast<const f64*>(mmap_addr_)[i * 2];
  }

  /// Interleaved 模式：获取第 i 个点的 y 坐标
  [[nodiscard]] inline f64 raw_y(u64 i) const noexcept {
    return static_cast<const f64*>(mmap_addr_)[i * 2 + 1];
  }

  /// 获取第 i 个点（两种模式通用）
  [[nodiscard]] inline Point point(u64 i) const noexcept {
    if (mode_ == Mode::Interleaved) {
      const auto* d = static_cast<const f64*>(mmap_addr_);
      return Point{ d[i * 2], d[i * 2 + 1] };
    }
    return Point{ x_[i], y_[i] };
  }

  [[nodiscard]] inline const std::string& path() const noexcept { return file_path_; }

private:
  bool read_points(int fd, u64 num_points, u64 file_offset = 0);
  void release();

private:
  std::string file_path_;
  Mode mode_{Mode::None};

  // SoA 存储（小文件）
  std::unique_ptr<f64[]> x_;
  std::unique_ptr<f64[]> y_;

  u64 num_points_{0};

  // mmap 信息（两种模式共享）
  void* mmap_addr_{nullptr};
  u64   mmap_length_{0};
};

#endif  // DATASET_HPP
