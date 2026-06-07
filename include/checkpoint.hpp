#ifndef CHECKPOINT_HPP
#define CHECKPOINT_HPP

/**
 * @file checkpoint.hpp
 * @brief 断点继续 — checkpoint 序列化/反序列化
 *
 * 格式（二进制，小端序）：
 *   [magic: u64] = 0x4B4D45414E535F43  ("KMEANS_C")
 *   [version: u32] = 1
 *   [k: u32]
 *   [iteration: i32]
 *   [centers: f64[k] * 2]   // cx, cy 交错
 *   [reserved: u64] = 0
 *
 * 文件路径由 KMeansConfig::checkpoint_path 指定。
 */

#include "types.hpp"

#include <cstdint>
#include <string>

/// Checkpoint magic number
constexpr u64 kCheckpointMagic = 0x4B4D45414E535F43ULL;  // "KMEANS_C"

/// Checkpoint 版本
constexpr u32 kCheckpointVersion = 1;

/// Checkpoint 头部
struct CheckpointHeader {
  u64 magic;       // 8
  u32 version;     // 4 -> 12
  u32 k;           // 4 -> 16
  i32 iteration;   // 4 -> 20
  u32 _pad;        // 4 -> 24
  u64 reserved;    // 8 -> 32
} __attribute__((packed));

static_assert(sizeof(CheckpointHeader) == 32,
              "CheckpointHeader must be 32 bytes");

/**
 * @brief 将 checkpoint 写入文件
 * @param path   文件路径
 * @param cx     中心 x 数组（长度 k）
 * @param cy     中心 y 数组（长度 k）
 * @param k      簇数
 * @param iteration 当前迭代轮次
 * @return true 成功
 */
[[nodiscard]] bool checkpoint_save(const std::string& path,
                                   const f64* cx, const f64* cy,
                                   u32 k, i32 iteration);

/**
 * @brief 从文件读取 checkpoint
 * @param path      文件路径
 * @param cx        输出中心 x 数组（长度 k）
 * @param cy        输出中心 y 数组（长度 k）
 * @param k         输出簇数
 * @param iteration 输出迭代轮次
 * @return true 成功
 */
[[nodiscard]] bool checkpoint_load(const std::string& path,
                                   f64* cx, f64* cy,
                                   u32& k, i32& iteration);

/**
 * @brief 检查 checkpoint 是否存在且有效
 */
[[nodiscard]] bool checkpoint_valid(const std::string& path);

#endif  // CHECKPOINT_HPP
