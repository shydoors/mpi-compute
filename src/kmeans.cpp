#include "kmeans.hpp"
#include "checkpoint.hpp"
#include "dataset.hpp"
#include "types.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <random>
#include <vector>
KMeans::KMeans(const KMeansConfig &config) : config_(config) {}
KMeans::~KMeans() = default;

KMeansResult KMeans::run() {
  using Clock = std::chrono::high_resolution_clock;
  KMeansResult result;
  auto t_start = Clock::now();
  /** 1. 加载数据 */
  auto t_load_start = Clock::now();
  Dataset data;
  bool loaded = false;
  if (config_.streaming) {
    loaded = data.load_streaming(config_.data_path, config_.stream_chunk_size);
  } else {
    loaded = data.load(config_.data_path, config_.use_mmap);
  }
  if (!loaded) {
    std::fprintf(stderr, "Error: 数据加载失败\n");
    return result;
  }
  auto t_load_end = Clock::now();
  result.time_load =
      std::chrono::duration<f64>(t_load_end - t_load_start).count();
  result.labels = std::make_unique<u32[]>(data.size());
  /** 2. 自动 K 推导 */
  auto t_autok_start = Clock::now();
  u32 k = config_.fixed_k;
  if (config_.auto_k) {
    /** 自动 K 推导*/
    std::printf("\n--- 自动 K 推导 ---\n");
    auto samples = data.uniform_sample(config_.sample_size);
    u64 sample_n = samples.size() / 2;
    if (sample_n < 10) {
      std::fprintf(stderr, "Error: 采样点太少 (%lu)\n", sample_n);
      return result;
    }
    Dataset sample_ds;
    auto samp_x = std::make_unique<f64[]>(sample_n);
    auto samp_y = std::make_unique<f64[]>(sample_n);
    for (u64 i = 0; i < sample_n; ++i) {
      samp_x[i] = samples[i * 2];
      samp_y[i] = samples[i * 2 + 1];
    }
    (void)sample_ds.from_memory(samp_x.release(), samp_y.release(), sample_n);
    k = auto_detect_k(sample_ds);
    std::printf("  自动推导 K = %u\n", k);
  } else if (k == 0) {
    k = 10;
    std::printf("  使用默认 K = %u\n", k);
  } else {
    std::printf("  使用指定 K = %u\n", k);
  }

  result.k = k;
  auto t_autok_end = Clock::now();
  result.time_autok =
      std::chrono::duration<f64>(t_autok_end - t_autok_start).count();
  /** 3. 迭代 */
  auto t_iter_start = Clock::now();
  switch (config_.backend) {
  case Backend::Sequential:
    result = run_sequential(data, k);
    break;
  case Backend::OpenMP:
    result = run_omp(data, k);
    break;
  case Backend::CUDA:
    result = run_cuda(data, k);
    break;
  }
  auto t_iter_end = Clock::now();
  result.time_iterate =
      std::chrono::duration<f64>(t_iter_end - t_iter_start).count();
  result.k = k;
  auto t_end = Clock::now();
  result.time_total = std::chrono::duration<f64>(t_end - t_start).count();
  return result;
}
u32 KMeans::auto_detect_k(const Dataset &sample) const {
  const u64 n = sample.size();
  if (n == 0) {
    return kMinClusters;
  }
  const u32 max_k_search = static_cast<u32>(
      std::min<u64>(std::sqrt(static_cast<f64>(n)), kMaxClusters));
  const u32 min_k = kMinClusters;
  std::vector<f64> sse_values;
  sse_values.reserve(max_k_search - min_k + 1);
  std::printf("  搜索 K: %u ~ %u, 样本点数: %lu\n", min_k, max_k_search, n);
  auto xs = sample.x();
  auto ys = sample.y();
  for (u32 kk = min_k; kk <= max_k_search; ++kk) {
    auto cx = std::make_unique<f64[]>(kk);
    auto cy = std::make_unique<f64[]>(kk);
    cx[0] = xs[0];
    cy[0] = ys[0];
    std::mt19937_64 rng(42);
    for (u32 c = 1; c < kk; ++c) {
      std::vector<f64> min_dists(n);
      f64 total_dist = 0.0;
      for (u64 i = 0; i < n; ++i) {
        f64 best = std::numeric_limits<f64>::max();
        for (u32 j = 0; j < c; ++j) {
          f64 dx = xs[i] - cx[j];
          f64 dy = ys[i] - cy[j];
          f64 d = dx * dx + dy * dy;
          if (d < best)
            best = d;
        }
        min_dists[i] = best;
        total_dist += best;
      }
      f64 threshold =
          total_dist * std::uniform_real_distribution<f64>(0, 1)(rng);
      f64 accum = 0.0;
      u64 selected = n - 1;
      for (u64 i = 0; i < n; ++i) {
        accum += min_dists[i];
        if (accum >= threshold) {
          selected = i;
          break;
        }
      }
      cx[c] = xs[selected];
      cy[c] = ys[selected];
    }
    auto labels = std::make_unique<u32[]>(n);
    constexpr i32 max_init_iter = 50;
    for (i32 iter = 0; iter < max_init_iter; ++iter) {
      auto sumx = std::make_unique<f64[]>(kk);
      auto sumy = std::make_unique<f64[]>(kk);
      auto cnt = std::make_unique<u64[]>(kk);
      for (u64 i = 0; i < n; ++i) {
        f64 best_dist = std::numeric_limits<f64>::max();
        u32 best_c = 0;
        for (u32 j = 0; j < kk; ++j) {
          f64 dx = xs[i] - cx[j];
          f64 dy = ys[i] - cy[j];
          f64 d = dx * dx + dy * dy;
          if (d < best_dist) {
            best_dist = d;
            best_c = j;
          }
        }
        labels[i] = best_c;
        sumx[best_c] += xs[i];
        sumy[best_c] += ys[i];
        cnt[best_c] += 1;
      }
      f64 delta = 0.0;
      for (u32 j = 0; j < kk; ++j) {
        if (cnt[j] > 0) {
          f64 new_cx = sumx[j] / static_cast<f64>(cnt[j]);
          f64 new_cy = sumy[j] / static_cast<f64>(cnt[j]);
          delta += (new_cx - cx[j]) * (new_cx - cx[j]) +
                   (new_cy - cy[j]) * (new_cy - cy[j]);
          cx[j] = new_cx;
          cy[j] = new_cy;
        }
      }
      if (delta < kConvergenceThreshold) {
        break;
      }
    }
    f64 sse = 0.0;
    for (u64 i = 0; i < n; ++i) {
      f64 dx = xs[i] - cx[labels[i]];
      f64 dy = ys[i] - cy[labels[i]];
      sse += dx * dx + dy * dy;
    }
    sse_values.push_back(sse);
    if (kk % 10 == 0 || kk == max_k_search) {
      f64 pct = 100.0 * (kk - min_k + 1) / (max_k_search - min_k + 1);
      std::printf("    K=%4u  SSE=%.6e  [%.0f%%]\n", kk, sse, pct);
    }
  }
  u32 m = static_cast<u32>(sse_values.size());
  if (m <= 2) {
    return min_k + (m > 1 ? 1 : 0);
  }
  std::vector<f64> x_norm(m);
  std::vector<f64> y_norm(m);
  for (u32 i = 0; i < m; ++i) {
    x_norm[i] = static_cast<f64>(i) / static_cast<f64>(m - 1);
  }
  f64 y_min = *std::min_element(sse_values.begin(), sse_values.end());
  f64 y_max = *std::max_element(sse_values.begin(), sse_values.end());
  f64 y_range = y_max - y_min;
  if (y_range < 1e-30) {
    return min_k;
  }
  for (u32 i = 0; i < m; ++i) {
    y_norm[i] = (sse_values[i] - y_min) / y_range;
  }
  u32 elbow_idx = 0;
  f64 max_dist = 0.0;
  for (u32 i = 1; i < m - 1; ++i) {
    f64 x0 = 0.0, y0 = 1.0;
    f64 x1 = 1.0, y1 = 0.0;
    f64 dist = std::abs((y1 - y0) * x_norm[i] - (x1 - x0) * y_norm[i] +
                        x1 * y0 - y1 * x0) /
               std::sqrt((y1 - y0) * (y1 - y0) + (x1 - x0) * (x1 - x0));
    if (dist > max_dist) {
      max_dist = dist;
      elbow_idx = i;
    }
  }
  return min_k + elbow_idx;
}

KMeansResult KMeans::run_sequential(Dataset &data, u32 k) {
  std::printf("\n--- Sequential Backend (验证用) ---\n");
  auto labels = std::make_unique<u32[]>(data.size());
  bool interleaved = (data.mode() == Dataset::Mode::Interleaved);
  const auto *xs = data.raw_data();
  /** 两种模式都能用：SoA 时 raw_data() 返回*/
  /** nullptr，但下面会区分*/
  const f64 *ys = nullptr;

  /** 初始化中心（前 k 个点）*/
  auto init_cx = std::make_unique<f64[]>(k);
  auto init_cy = std::make_unique<f64[]>(k);
  for (u32 j = 0; j < k; ++j) {
    auto p = data.point(j);
    init_cx[j] = p.x;
    init_cy[j] = p.y;
  }

  if (interleaved) {
    xs = data.raw_data();
  } else {
    xs = data.x();
    ys = data.y();
  }

  auto result = run_kmeans_sequential(
      xs, ys, data.size(), k, config_.max_iterations, config_.threshold,
      init_cx.get(), init_cy.get(), interleaved, labels.get());
  result.labels = std::move(labels);
  return result;
}
KMeansResult KMeans::run_omp(Dataset &data, u32 k) {
  std::printf("\n--- OpenMP Backend ---\n");
  auto labels = std::make_unique<u32[]>(data.size());
  bool interleaved = (data.mode() == Dataset::Mode::Interleaved);
  const f64 *xs = nullptr;
  const f64 *ys = nullptr;
  auto init_cx = std::make_unique<f64[]>(k);
  auto init_cy = std::make_unique<f64[]>(k);
  for (u32 j = 0; j < k; ++j) {
    auto p = data.point(j);
    init_cx[j] = p.x;
    init_cy[j] = p.y;
  }
  if (interleaved) {
    xs = data.raw_data();
  } else {
    xs = data.x();
    ys = data.y();
  }
  auto result = run_kmeans_omp(xs, ys, data.size(), k, config_.max_iterations,
                               config_.threshold, init_cx.get(), init_cy.get(),
                               interleaved, labels.get());
  result.labels = std::move(labels);
  return result;
}
__attribute__((weak)) KMeansResult KMeans::run_cuda(Dataset & /*data*/, u32 k) {
  std::fprintf(stderr, "Error: CUDA backend not linked. "
                       "Please compile with nvcc (kmeans_kernels.cu).\n");
  KMeansResult result;
  result.k = k;
  return result;
}
bool KMeans::save_checkpoint(i32 iteration, const f64 *cx, const f64 *cy,
                             u32 k) {
  if (config_.checkpoint_path.empty()) {
    return false;
  }
  return checkpoint_save(config_.checkpoint_path, cx, cy, k, iteration);
}
bool KMeans::load_checkpoint(f64 *cx, f64 *cy, u32 &k, i32 &iteration) {
  if (config_.checkpoint_path.empty()) {
    return false;
  }
  return checkpoint_load(config_.checkpoint_path, cx, cy, k, iteration);
}
