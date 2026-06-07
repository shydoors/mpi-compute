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
  Dataset() = default;

  // 禁止拷贝
  Dataset(const Dataset&) = delete;
  Dataset& operator=(const Dataset&) = delete;

  // 允许移动
  Dataset(Dataset&&) noexcept = default;
  Dataset& operator=(Dataset&&) noexcept = default;

  ~Dataset();

  // ----------------------------------------------------------
  // 加载方式
  // ----------------------------------------------------------

  /// 全量加载：使用 mmap 或 read 将整个文件读入内存
  /// @param path       数据文件路径（相对路径，项目根为基准）
  /// @param use_mmap   是否使用 mmap（大文件推荐）
  /// @return true 成功
  [[nodiscard]] bool load(const std::string& path, bool use_mmap = true);

  /// 流式分块读取（用于内存受限场景）
  /// @param path       数据文件路径
  /// @param chunk_size 每块字节数（默认 64 MiB）
  /// @return true 成功
  [[nodiscard]] bool load_streaming(const std::string& path,
                                    u64 chunk_size = 64ULL * 1024 * 1024);

  /// 从已有内存指针恢复（断点继续用）
  [[nodiscard]] bool from_memory(f64* x_data, f64* y_data, u64 num_points);

  // ----------------------------------------------------------
  // 采样
  // ----------------------------------------------------------

  /// 均匀采样
  /// @param sample_size 目标采样点数
  /// @return 采样点的 vector（x,y 交错存储）
  [[nodiscard]] std::vector<f64> uniform_sample(u64 sample_size = kDefaultSampleSize) const;

  // ----------------------------------------------------------
  // 访问器
  // ----------------------------------------------------------

  [[nodiscard]] inline u64 size()  const noexcept { return num_points_; }
  [[nodiscard]] inline u64 bytes() const noexcept { return num_points_ * kBytesPerPoint; }
  [[nodiscard]] inline bool empty() const noexcept { return num_points_ == 0; }
  [[nodiscard]] inline bool is_mmap() const noexcept { return is_mmap_; }

  [[nodiscard]] inline const f64* x() const noexcept { return x_.get(); }
  [[nodiscard]] inline const f64* y() const noexcept { return y_.get(); }
  [[nodiscard]] inline f64*       x_mut() noexcept { return x_.get(); }
  [[nodiscard]] inline f64*       y_mut() noexcept { return y_.get(); }

  /// 获取第 i 个点（边界检查仅在 debug 模式）
  [[nodiscard]] inline Point point(u64 i) const noexcept {
    return Point{ x_[i], y_[i] };
  }

  /// 获取文件名
  [[nodiscard]] inline const std::string& path() const noexcept { return file_path_; }

private:
  /// 内部：从已打开的 fd 读入点数据
  [[nodiscard]] bool read_points(int fd, u64 num_points, u64 file_offset = 0);

  /// 内部：释放资源
  void release();

private:
  std::string file_path_;

  // SoA 分离存储 — 使用 unique_ptr<f64[]> 而非 vector 避免默认初始化开销
  std::unique_ptr<f64[]> x_;
  std::unique_ptr<f64[]> y_;

  u64 num_points_{0};
  bool is_mmap_{false};

  // mmap 相关
  void* mmap_addr_{nullptr};
  u64   mmap_length_{0};
};

#endif  // DATASET_HPP
