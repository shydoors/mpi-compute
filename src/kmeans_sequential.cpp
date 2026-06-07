/**
 * @file kmeans_sequential.cpp
 * @brief 串行 CPU K-Means 实现（用于正确性验证）
 *
 * 算法：标准 Lloyd K-Means
 * 并行模型：无（纯串行，与 OpenMP / CUDA 版本对比基准）
 */

#include "kmeans.hpp"
#include "types.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>

KMeansResult run_kmeans_sequential(
    const f64* xs, const f64* ys, u64 n, u32 k,
    i32 max_iter, f64 threshold,
    f64* init_cx, f64* init_cy,
    const std::string& checkpoint_path,
    i32 checkpoint_interval,
    bool resume,
    u32* labels_out)
{
  KMeansResult result;
  result.k = k;

  // 中心点
  auto cx = std::make_unique<f64[]>(k);
  auto cy = std::make_unique<f64[]>(k);

  // 初始中心
  for (u32 j = 0; j < k; ++j) {
    cx[j] = init_cx[j];
    cy[j] = init_cy[j];
  }

  // 断点继续：尝试加载 checkpoint
  i32 start_iter = 0;
  if (resume && !checkpoint_path.empty()) {
    u32 ck_k = k;
    i32 ck_iter = 0;
    if (checkpoint_load(checkpoint_path, cx.get(), cy.get(), ck_k, ck_iter)) {
      if (ck_k == k) {
        start_iter = ck_iter + 1;
        std::printf("  从 checkpoint 恢复: 迭代 %d\n", start_iter);
      } else {
        std::printf("  Warning: checkpoint K(%u) != current K(%u), 忽略\n", ck_k, k);
      }
    }
  }

  // 迭代
  for (i32 iter = start_iter; iter < max_iter; ++iter) {
    // 局部累加器
    auto sumx = std::make_unique<f64[]>(k);
    auto sumy = std::make_unique<f64[]>(k);
    auto cnt  = std::make_unique<u64[]>(k);

    // --- Map: 分配 & 局部聚合 ---
    for (u64 i = 0; i < n; ++i) {
      f64 best_dist = std::numeric_limits<f64>::max();
      u32 best_c = 0;

      for (u32 j = 0; j < k; ++j) {
        f64 dx = xs[i] - cx[j];
        f64 dy = ys[i] - cy[j];
        f64 d = dx * dx + dy * dy;
        if (d < best_dist) {
          best_dist = d;
          best_c = j;
        }
      }

      labels_out[i] = best_c;
      sumx[best_c] += xs[i];
      sumy[best_c] += ys[i];
      cnt[best_c] += 1;
    }

    // --- Reduce: 归约 & 更新中心 ---
    f64 delta = 0.0;
    for (u32 j = 0; j < k; ++j) {
      if (cnt[j] > 0) {
        f64 new_cx = sumx[j] / static_cast<f64>(cnt[j]);
        f64 new_cy = sumy[j] / static_cast<f64>(cnt[j]);
        delta += (new_cx - cx[j]) * (new_cx - cx[j])
               + (new_cy - cy[j]) * (new_cy - cy[j]);
        cx[j] = new_cx;
        cy[j] = new_cy;
      }
    }

    // 进度报告
    if ((iter + 1) % 10 == 0 || iter == 0) {
      std::printf("  Iter %3d / %d  delta=%.6e\n",
                  iter + 1, max_iter, delta);
    }

    // Checkpoint
    if (!checkpoint_path.empty() && checkpoint_interval > 0 &&
        (iter + 1) % checkpoint_interval == 0) {
      if (checkpoint_save(checkpoint_path, cx.get(), cy.get(), k, iter)) {
        std::printf("  Checkpoint 已保存 (iter %d)\n", iter);
      }
    }

    // 收敛检测
    if (delta < threshold) {
      std::printf("  收敛于迭代 %d (delta=%.6e)\n", iter, delta);
      result.iterations = iter + 1;
      break;
    }
  }

  if (result.iterations == 0) {
    result.iterations = max_iter;
  }

  // 计算 SSE
  f64 inertia = 0.0;
  for (u64 i = 0; i < n; ++i) {
    u32 c = labels_out[i];
    f64 dx = xs[i] - cx[c];
    f64 dy = ys[i] - cy[c];
    inertia += dx * dx + dy * dy;
  }
  result.inertia = inertia;

  // 输出中心
  result.centers_x = std::move(cx);
  result.centers_y = std::move(cy);

  return result;
}
