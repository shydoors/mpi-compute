/**
 * @file kmeans_omp.cpp
 * @brief OpenMP 并行 K-Means 实现
 *
 * 并行模型：Fork-Join (MapReduce)
 *   - Fork: 将 N 个点等分为 T 块（T = OMP 线程数）
 *   - Map: 每块独立计算点到所有中心的距离、分配簇、局部累加
 *   - Join: 归约 T 组局部累加器，更新中心
 *
 * 优化：
 *   - 线程局部累加器（无锁，消除 false sharing）
 *   - #pragma omp parallel for reduction 归约
 *   - SoA 布局（x/y分离）利于缓存
 */

#include "kmeans.hpp"
#include "types.hpp"
#include "checkpoint.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <omp.h>
#include <cstring>

KMeansResult run_kmeans_omp(
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
  std::memcpy(cx.get(), init_cx, k * sizeof(f64));
  std::memcpy(cy.get(), init_cy, k * sizeof(f64));

  // 断点继续
  i32 start_iter = 0;
  if (resume && !checkpoint_path.empty()) {
    u32 ck_k = k;
    i32 ck_iter = 0;
    if (checkpoint_load(checkpoint_path, cx.get(), cy.get(), ck_k, ck_iter)) {
      if (ck_k == k) {
        start_iter = ck_iter + 1;
        std::printf("  [OMP] 从 checkpoint 恢复: 迭代 %d\n", start_iter);
      }
    }
  }

  // 获取线程数
  i32 num_threads = omp_get_max_threads();
  std::printf("  [OMP] 线程数: %d\n", num_threads);

  // 迭代
  for (i32 iter = start_iter; iter < max_iter; ++iter) {
    // 每线程局部累加器（避免 false sharing: 每个线程分配独立缓存行对齐的数组）
    constexpr u64 kCacheLinePadding = 64 / sizeof(f64);
    const u64 padded_k = k + kCacheLinePadding;

    auto all_sumx = std::make_unique<f64[]>(num_threads * padded_k);
    auto all_sumy = std::make_unique<f64[]>(num_threads * padded_k);
    auto all_cnt  = std::make_unique<u64[]>(num_threads * k);  // u64 不需要 padding

    std::memset(all_sumx.get(), 0, num_threads * padded_k * sizeof(f64));
    std::memset(all_sumy.get(), 0, num_threads * padded_k * sizeof(f64));
    std::memset(all_cnt.get(),  0, num_threads * k * sizeof(u64));

    // --- Map: 分配 & 局部聚合 (Fork) ---
    #pragma omp parallel
    {
      i32 tid = omp_get_thread_num();
      f64* local_sumx = all_sumx.get() + tid * padded_k;
      f64* local_sumy = all_sumy.get() + tid * padded_k;
      u64* local_cnt  = all_cnt.get()  + tid * k;

      #pragma omp for schedule(static)
      for (u64 i = 0; i < n; ++i) {
        f64 best_dist = std::numeric_limits<f64>::max();
        u32 best_c = 0;

        // 到所有中心的距离（编译器自动向量化）
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
        local_sumx[best_c] += xs[i];
        local_sumy[best_c] += ys[i];
        local_cnt[best_c] += 1;
      }
    }

    // --- Join: 归约 & 更新中心 ---
    f64 delta = 0.0;
    for (u32 j = 0; j < k; ++j) {
      f64 sx = 0.0, sy = 0.0;
      u64 cnt_total = 0;

      for (i32 t = 0; t < num_threads; ++t) {
        sx += all_sumx[t * padded_k + j];
        sy += all_sumy[t * padded_k + j];
        cnt_total += all_cnt[t * k + j];
      }

      if (cnt_total > 0) {
        f64 new_cx = sx / static_cast<f64>(cnt_total);
        f64 new_cy = sy / static_cast<f64>(cnt_total);
        delta += (new_cx - cx[j]) * (new_cx - cx[j])
               + (new_cy - cy[j]) * (new_cy - cy[j]);
        cx[j] = new_cx;
        cy[j] = new_cy;
      }
    }

    // 进度
    if ((iter + 1) % 5 == 0 || iter == 0) {
      std::printf("  [OMP] Iter %3d / %d  delta=%.6e\n",
                  iter + 1, max_iter, delta);
    }

    // Checkpoint
    if (!checkpoint_path.empty() && checkpoint_interval > 0 &&
        (iter + 1) % checkpoint_interval == 0) {
      if (checkpoint_save(checkpoint_path, cx.get(), cy.get(), k, iter)) {
        std::printf("  [OMP] Checkpoint saved (iter %d)\n", iter);
      }
    }

    // 收敛
    if (delta < threshold) {
      std::printf("  [OMP] 收敛于迭代 %d (delta=%.6e)\n", iter, delta);
      result.iterations = iter + 1;
      break;
    }
  }

  if (result.iterations == 0) {
    result.iterations = max_iter;
  }

  // 计算 SSE
  f64 inertia = 0.0;
  #pragma omp parallel for reduction(+:inertia) schedule(static)
  for (u64 i = 0; i < n; ++i) {
    u32 c = labels_out[i];
    f64 dx = xs[i] - cx[c];
    f64 dy = ys[i] - cy[c];
    inertia += dx * dx + dy * dy;
  }
  result.inertia = inertia;

  result.centers_x = std::move(cx);
  result.centers_y = std::move(cy);

  return result;
}
