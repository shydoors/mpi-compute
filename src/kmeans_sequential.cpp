#include "checkpoint.hpp"
#include "kmeans.hpp"
#include "types.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>

/// 访问 xs[i] / ys[i]（兼容 SoA 和交错模式）
static inline f64 get_x(const f64 *xs, const f64 *ys, u64 i, bool interleaved) {
  (void)ys;
  return interleaved ? xs[i * 2] : xs[i];
}
static inline f64 get_y(const f64 *xs, const f64 *ys, u64 i, bool interleaved) {
  return interleaved ? xs[i * 2 + 1] : ys[i];
}

KMeansResult run_kmeans_sequential(const f64 *xs, const f64 *ys, u64 n, u32 k,
                                   i32 max_iter, f64 threshold, f64 *init_cx,
                                   f64 *init_cy, bool interleaved,
                                   u32 *labels_out) {
  KMeansResult result;
  result.k = k;

  auto cx = std::make_unique<f64[]>(k);
  auto cy = std::make_unique<f64[]>(k);
  for (u32 j = 0; j < k; ++j) {
    cx[j] = init_cx[j];
    cy[j] = init_cy[j];
  }

  for (i32 iter = 0; iter < max_iter; ++iter) {
    auto sumx = std::make_unique<f64[]>(k);
    auto sumy = std::make_unique<f64[]>(k);
    auto cnt = std::make_unique<u64[]>(k);

    for (u64 i = 0; i < n; ++i) {
      f64 px = get_x(xs, ys, i, interleaved);
      f64 py = get_y(xs, ys, i, interleaved);
      f64 best_dist = std::numeric_limits<f64>::max();
      u32 best_c = 0;

      for (u32 j = 0; j < k; ++j) {
        f64 dx = px - cx[j];
        f64 dy = py - cy[j];
        f64 d = dx * dx + dy * dy;
        if (d < best_dist) {
          best_dist = d;
          best_c = j;
        }
      }

      labels_out[i] = best_c;
      sumx[best_c] += px;
      sumy[best_c] += py;
      cnt[best_c] += 1;
    }

    f64 delta = 0.0;
    for (u32 j = 0; j < k; ++j) {
      if (cnt[j] > 0) {
        f64 new_cx = sumx[j] / static_cast<f64>(cnt[j]);
        f64 new_cy = sumy[j] / static_cast<f64>(cnt[j]);
        delta += (new_cx - cx[j]) * (new_cx - cx[j]) +
                 (new_cy - cy[j]) * (new_cy - cy[j]);
        cx[j] = new_cx;
        cy[j] = new_cy;
      }
    }

    if ((iter + 1) % 10 == 0 || iter == 0)
      std::printf("  Iter %3d / %d  delta=%.6e\n", iter + 1, max_iter, delta);

    if (delta < threshold) {
      std::printf("  收敛于迭代 %d (delta=%.6e)\n", iter, delta);
      result.iterations = iter + 1;
      break;
    }
  }

  if (result.iterations == 0)
    result.iterations = max_iter;

  f64 inertia = 0.0;
  for (u64 i = 0; i < n; ++i) {
    u32 c = labels_out[i];
    f64 dx = get_x(xs, ys, i, interleaved) - cx[c];
    f64 dy = get_y(xs, ys, i, interleaved) - cy[c];
    inertia += dx * dx + dy * dy;
  }
  result.inertia = inertia;

  result.centers_x = std::move(cx);
  result.centers_y = std::move(cy);

  return result;
}
