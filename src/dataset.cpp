/**
 * @file dataset.cpp
 * @brief 数据集管理实现
 *
 * 加载策略：
 *   小文件（< 1 GiB）→ mmap + SoA 转换（兼容所有后端）
 *   大文件（>= 1 GiB）→ mmap 零拷贝，保留交错数据（不 SoA，省内存）
 */

#include "dataset.hpp"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <random>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// ============================================================
// 大文件阈值（1 GiB）
// ============================================================
namespace {
constexpr u64 kLargeFileThreshold = 1ULL * 1024 * 1024 * 1024;  // 1 GiB

u64 file_size(int fd) {
  struct stat st;
  if (::fstat(fd, &st) != 0) {
    std::fprintf(stderr, "Error: fstat failed: %s\n", std::strerror(errno));
    return 0;
  }
  return static_cast<u64>(st.st_size);
}

bool read_exact(int fd, void* buf, u64 count) {
  auto* ptr = static_cast<char*>(buf);
  while (count > 0) {
    auto nread = ::read(fd, ptr, count);
    if (nread < 0) {
      if (errno == EINTR) continue;
      std::fprintf(stderr, "Error: read failed: %s\n", std::strerror(errno));
      return false;
    }
    if (nread == 0) {
      std::fprintf(stderr, "Error: unexpected EOF\n");
      return false;
    }
    ptr   += nread;
    count -= static_cast<u64>(nread);
  }
  return true;
}

}  // anonymous namespace

// ============================================================
// Dataset 实现
// ============================================================

Dataset::~Dataset() {
  release();
}

void Dataset::release() {
  if (mmap_addr_ != nullptr && mmap_addr_ != MAP_FAILED) {
    ::munmap(mmap_addr_, mmap_length_);
  }
  mmap_addr_ = nullptr;
  mmap_length_ = 0;
  x_.reset();
  y_.reset();
  num_points_ = 0;
  mode_ = Mode::None;
}

// ----------------------------------------------------------
// 全量加载
// ----------------------------------------------------------
bool Dataset::load(const std::string& path, bool use_mmap) {
  release();
  file_path_ = path;

  int fd = ::open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    std::fprintf(stderr, "Error: 无法打开文件 '%s': %s\n",
                 path.c_str(), std::strerror(errno));
    return false;
  }

  u64 fsize = file_size(fd);
  if (fsize == 0) {
    std::fprintf(stderr, "Error: 文件 '%s' 为空\n", path.c_str());
    ::close(fd);
    return false;
  }

  if (fsize % kBytesPerPoint != 0) {
    std::fprintf(stderr, "Warning: 文件大小 %lu 不是 %zu 的整数倍, 尾部数据将被忽略\n",
                 fsize, kBytesPerPoint);
  }

  u64 npoints = fsize / kBytesPerPoint;

  // ---- 根据文件大小决定加载模式 ----
  bool use_interleaved = use_mmap && (fsize >= kLargeFileThreshold);

  std::printf("  加载 %lu 个点 (%s, %s)\n", npoints,
              use_mmap ? "mmap" : "read",
              use_interleaved ? "交错零拷贝" : "SoA 分离");

  if (use_interleaved) {
    // === 大文件：mmap + 交错零拷贝 ===
    void* addr = ::mmap(nullptr, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
      std::fprintf(stderr, "Warning: mmap 失败 (%s), 回退到 read\n",
                   std::strerror(errno));
      ::close(fd);
      return false;
    }
    mmap_addr_   = addr;
    mmap_length_ = fsize;
    mode_        = Mode::Interleaved;
    num_points_  = npoints;
    ::close(fd);

    std::printf("  加载完成: %lu 点, %.2f GiB (交错零拷贝)\n",
                num_points_, fsize / (1024.0 * 1024.0 * 1024.0));
    return true;
  }

  // === 小文件：mmap + SoA 转换 ===
  auto x_tmp = std::make_unique<f64[]>(npoints);
  auto y_tmp = std::make_unique<f64[]>(npoints);

  if (use_mmap) {
    void* addr = ::mmap(nullptr, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
      std::fprintf(stderr, "Warning: mmap 失败 (%s), 回退到 read\n",
                   std::strerror(errno));
      ::lseek(fd, 0, SEEK_SET);
      if (!read_points(fd, npoints, 0)) {
        ::close(fd);
        return false;
      }
      mode_ = Mode::SoA;
    } else {
      const auto* data = static_cast<const f64*>(addr);
      for (u64 i = 0; i < npoints; ++i) {
        x_tmp[i] = data[i * 2];
        y_tmp[i] = data[i * 2 + 1];
      }
      // mmap 用完后立即释放（数据已复制到 SoA）
      ::munmap(addr, fsize);
      x_ = std::move(x_tmp);
      y_ = std::move(y_tmp);
      mode_ = Mode::SoA;
    }
  } else {
    ::lseek(fd, 0, SEEK_SET);
    if (!read_points(fd, npoints, 0)) {
      ::close(fd);
      return false;
    }
    mode_ = Mode::SoA;
  }

  ::close(fd);
  num_points_ = npoints;

  std::printf("  加载完成: %lu 点, %.2f MiB\n",
              num_points_, fsize / (1024.0 * 1024.0));
  return true;
}

// ----------------------------------------------------------
// read_points
// ----------------------------------------------------------
bool Dataset::read_points(int fd, u64 npoints, u64 /*file_offset*/) {
  auto x_tmp = std::make_unique<f64[]>(npoints);
  auto y_tmp = std::make_unique<f64[]>(npoints);

  f64 buf[2];
  for (u64 i = 0; i < npoints; ++i) {
    if (!read_exact(fd, buf, sizeof(buf))) {
      std::fprintf(stderr, "Error: 读取点 %lu/%lu 时失败\n", i, npoints);
      return false;
    }
    x_tmp[i] = buf[0];
    y_tmp[i] = buf[1];
  }

  x_ = std::move(x_tmp);
  y_ = std::move(y_tmp);
  return true;
}

// ----------------------------------------------------------
// 流式加载
// ----------------------------------------------------------
bool Dataset::load_streaming(const std::string& path, u64 chunk_size) {
  int fd = ::open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    std::fprintf(stderr, "Error: 无法打开文件 '%s': %s\n",
                 path.c_str(), std::strerror(errno));
    return false;
  }

  u64 fsize = file_size(fd);
  if (fsize == 0) { ::close(fd); return false; }

  u64 npoints = fsize / kBytesPerPoint;
  file_path_ = path;

  std::printf("  流式读取: %lu 点, 块大小 %lu MiB\n",
              npoints, chunk_size / (1024 * 1024));

  auto x_tmp = std::make_unique<f64[]>(npoints);
  auto y_tmp = std::make_unique<f64[]>(npoints);

  ::lseek(fd, 0, SEEK_SET);
  for (u64 i = 0; i < npoints; ++i) {
    f64 buf[2];
    if (!read_exact(fd, buf, sizeof(buf))) {
      std::fprintf(stderr, "Error: 流式读取点 %lu/%lu 失败\n", i, npoints);
      ::close(fd);
      return false;
    }
    x_tmp[i] = buf[0];
    y_tmp[i] = buf[1];

    if ((i + 1) % 10'000'000 == 0) {
      std::printf("  已读取: %lu / %lu 点 (%.1f%%)\n",
                  i + 1, npoints, 100.0 * (i + 1) / npoints);
    }
  }

  ::close(fd);

  x_ = std::move(x_tmp);
  y_ = std::move(y_tmp);
  num_points_ = npoints;
  mode_ = Mode::SoA;

  std::printf("  流式读取完成: %lu 点\n", num_points_);
  return true;
}

// ----------------------------------------------------------
// from_memory
// ----------------------------------------------------------
bool Dataset::from_memory(f64* x_data, f64* y_data, u64 num_points) {
  release();
  x_.reset(x_data);
  y_.reset(y_data);
  num_points_ = num_points;
  mode_ = Mode::SoA;
  return true;
}

// ----------------------------------------------------------
// 均匀采样
// ----------------------------------------------------------
std::vector<f64> Dataset::uniform_sample(u64 sample_size) const {
  if (empty()) return {};

  if (num_points_ <= sample_size) {
    std::vector<f64> result;
    result.reserve(num_points_ * 2);
    for (u64 i = 0; i < num_points_; ++i) {
      auto p = point(i);
      result.push_back(p.x);
      result.push_back(p.y);
    }
    return result;
  }

  u64 step = num_points_ / sample_size;
  if (step < 1) step = 1;

  u64 actual_samples = num_points_ / step;
  if (actual_samples > sample_size) {
    step = num_points_ / sample_size;
    actual_samples = num_points_ / step;
  }

  std::vector<f64> result;
  result.reserve(actual_samples * 2);

  for (u64 i = 0; i < num_points_; i += step) {
    auto p = point(i);
    result.push_back(p.x);
    result.push_back(p.y);
  }

  std::printf("  采样: %zu 点 (步长 %lu, 总 %lu 点)\n",
              result.size() / 2, step, num_points_);
  return result;
}
