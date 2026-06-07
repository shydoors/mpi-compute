/**
 * @file checkpoint.cpp
 * @brief 断点继续 — checkpoint 序列化/反序列化实现
 *
 * 格式：
 *   [CheckpointHeader: 32 bytes]
 *   [中心点数据: f64[k] + f64[k] = 16*k bytes]
 */

#include "checkpoint.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

// ============================================================
// 写 checkpoint
// ============================================================
bool checkpoint_save(const std::string& path,
                     const f64* cx, const f64* cy,
                     u32 k, i32 iteration)
{
  std::FILE* fp = std::fopen(path.c_str(), "wb");
  if (!fp) {
    std::fprintf(stderr, "Error: 无法创建 checkpoint 文件 '%s'\n", path.c_str());
    return false;
  }

  // 写头部
  CheckpointHeader header;
  header.magic     = kCheckpointMagic;
  header.version   = kCheckpointVersion;
  header.k         = k;
  header.iteration = iteration;
  header.reserved  = 0;

  if (std::fwrite(&header, sizeof(header), 1, fp) != 1) {
    std::fprintf(stderr, "Error: 写入 checkpoint 头部失败\n");
    std::fclose(fp);
    return false;
  }

  // 写中心点（交错：cx[0], cy[0], cx[1], cy[1], ...）
  // 或简单分两块写
  if (std::fwrite(cx, sizeof(f64), k, fp) != k) {
    std::fprintf(stderr, "Error: 写入 checkpoint cx 失败\n");
    std::fclose(fp);
    return false;
  }
  if (std::fwrite(cy, sizeof(f64), k, fp) != k) {
    std::fprintf(stderr, "Error: 写入 checkpoint cy 失败\n");
    std::fclose(fp);
    return false;
  }

  std::fclose(fp);
  return true;
}

// ============================================================
// 读 checkpoint
// ============================================================
bool checkpoint_load(const std::string& path,
                     f64* cx, f64* cy,
                     u32& k, i32& iteration)
{
  std::FILE* fp = std::fopen(path.c_str(), "rb");
  if (!fp) {
    std::fprintf(stderr, "Warning: 无法打开 checkpoint 文件 '%s'\n", path.c_str());
    return false;
  }

  // 读头部
  CheckpointHeader header;
  if (std::fread(&header, sizeof(header), 1, fp) != 1) {
    std::fprintf(stderr, "Error: 读取 checkpoint 头部失败\n");
    std::fclose(fp);
    return false;
  }

  // 校验 magic
  if (header.magic != kCheckpointMagic) {
    std::fprintf(stderr, "Error: checkpoint magic 不匹配 (expected %016lx, got %016lx)\n",
                 kCheckpointMagic, header.magic);
    std::fclose(fp);
    return false;
  }

  // 校验版本
  if (header.version != kCheckpointVersion) {
    std::fprintf(stderr, "Error: checkpoint 版本不匹配 (expected %u, got %u)\n",
                 kCheckpointVersion, header.version);
    std::fclose(fp);
    return false;
  }

  k = header.k;
  iteration = header.iteration;

  // 读中心点
  if (std::fread(cx, sizeof(f64), k, fp) != k) {
    std::fprintf(stderr, "Error: 读取 checkpoint cx 失败\n");
    std::fclose(fp);
    return false;
  }
  if (std::fread(cy, sizeof(f64), k, fp) != k) {
    std::fprintf(stderr, "Error: 读取 checkpoint cy 失败\n");
    std::fclose(fp);
    return false;
  }

  std::fclose(fp);
  return true;
}

// ============================================================
// 检查 checkpoint 有效性
// ============================================================
bool checkpoint_valid(const std::string& path) {
  std::FILE* fp = std::fopen(path.c_str(), "rb");
  if (!fp) return false;

  CheckpointHeader header;
  if (std::fread(&header, sizeof(header), 1, fp) != 1) {
    std::fclose(fp);
    return false;
  }

  std::fclose(fp);
  return (header.magic == kCheckpointMagic &&
          header.version == kCheckpointVersion);
}
