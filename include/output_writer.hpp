#ifndef OUTPUT_WRITER_HPP
#define OUTPUT_WRITER_HPP

/**
 * @file output_writer.hpp
 * @brief 结果输出 -- 声明
 *
 * 实现文件: src/output_writer.cpp
 */

#include "kmeans.hpp"
#include "types.hpp"

#include <string>

// ============================================================
// 控制台输出
// ============================================================

/// 打印 K-Means 结果到 stdout
void output_print_result(const KMeansResult& result);

/// 打印配置信息到 stdout
void output_print_config(const KMeansConfig& config, u64 render_max);

// ============================================================
// 文件输出
// ============================================================

/// 保存 result.txt 日志
void output_save_result_txt(const char* dir,
                             const KMeansConfig& config,
                             const KMeansResult& result,
                             const char* timestamp);

/// 保存 labels.bin（二进制格式）
void output_save_labels_bin(const char* dir,
                             const KMeansResult& result,
                             u64 num_points);

/// 保存 centers.csv
void output_save_centers_csv(const char* dir,
                              const KMeansResult& result);

/// 保存 render.csv（超过 max_points 时均匀采样）
void output_save_render_csv(const char* dir,
                             const KMeansConfig& config,
                             const KMeansResult& result,
                             u64 num_points,
                             u64 max_render_points);

#endif  // OUTPUT_WRITER_HPP
