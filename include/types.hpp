#ifndef TYPES_HPP
#define TYPES_HPP

/**
 * @file types.hpp
 * @brief 项目全局类型定义
 *
 * 遵循规范：
 *   - 使用 <cstdint> 定宽整数
 *   - 禁止全局 using namespace
 *   - 优先 const/constexpr 而非 #define
 */

#include <cstdint>
#include <cstddef>

// ============================================================
// 定宽整数类型别名（遵循规范：语义准确）
// ============================================================
using i32  = std::int32_t;
using u32  = std::uint32_t;
using i64  = std::int64_t;
using u64  = std::uint64_t;
using f64  = double;
using usize = std::size_t;

// ============================================================
// 点类型（2D）
// ============================================================
struct Point {
  f64 x;
  f64 y;
} __attribute__((aligned(16)));  // 16 字节对齐，一个 cache line 可容纳 4 个

// ============================================================
// 簇中心
// ============================================================
struct Center {
  f64 x;
  f64 y;
  u64 count;  // 属于该簇的点数
} __attribute__((aligned(32)));

// ============================================================
// 全局常量
// ============================================================
constexpr u64   kDefaultSampleSize    = 1'000'000;
constexpr u32   kMaxClusters          = 4'000;
constexpr u32   kMinClusters          = 2;
constexpr i32   kMaxIterations        = 300;
constexpr f64   kConvergenceThreshold = 1e-8;

// 文件格式常量：二进制 double[2] 序列
constexpr usize kBytesPerPoint = sizeof(f64) * 2;  // 16

#endif  // TYPES_HPP
