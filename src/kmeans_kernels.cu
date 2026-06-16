#include "checkpoint.hpp"
#include "kmeans.hpp"
#include "kmeans_kernels.cuh"
#include "types.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cuda_runtime.h>
#include <limits>
#include <memory>
#include <random>

// ============================================================
// 不依赖宏的错误处理（nvcc 友好）
// ============================================================
static inline bool cuda_ok(cudaError_t err, const char *file, int line) {
  if (err != cudaSuccess) {
    std::fprintf(stderr, "CUDA Error at %s:%d: %s\n", file, line,
                 cudaGetErrorString(err));
    return false;
  }
  return true;
}
#define CUDA_OK(expr) cuda_ok((expr), __FILE__, __LINE__)
// 每 chunk 64 MB 交错数据 = 4M 点
static constexpr u64 kChunkPoints = 4ULL * 1024 * 1024;
// ============================================================
// Kernel: 分配 + 局部聚合一趟完成
// ============================================================
__global__ void
assign_kernel_stream(const f64 *__restrict__ d_x, const f64 *__restrict__ d_y,
                     const f64 *__restrict__ d_cx, const f64 *__restrict__ d_cy,
                     u32 *__restrict__ d_labels, f64 *__restrict__ d_sumx_out,
                     f64 *__restrict__ d_sumy_out, u64 *__restrict__ d_cnt_out,
                     u64 n_chunk, u32 k) {
  extern __shared__ f64 s_centers[];
  f64 *s_cx = s_centers;
  f64 *s_cy = s_centers + k;
  i32 tid = threadIdx.x;
  for (int j = tid; j < (int)k; j += blockDim.x) {
    s_cx[j] = d_cx[j];
    s_cy[j] = d_cy[j];
  }
  __syncthreads();

  u64 tile_start = (u64)blockIdx.x * blockDim.x;
  u64 tile_end = min(tile_start + blockDim.x, n_chunk);

  for (u64 i = tile_start + tid; i < tile_end; i += blockDim.x) {
    f64 px = d_x[i];
    f64 py = d_y[i];
    f64 best_dist = 1e308;
    u32 best_c = 0;
#pragma unroll 4
    for (u32 j = 0; j < k; ++j) {
      f64 dx = px - s_cx[j];
      f64 dy = py - s_cy[j];
      f64 d = dx * dx + dy * dy;
      if (d < best_dist) {
        best_dist = d;
        best_c = j;
      }
    }

    d_labels[i] = best_c;
    atomicAdd(&d_sumx_out[best_c], px);
    atomicAdd(&d_sumy_out[best_c], py);
    atomicAdd((unsigned long long *)&d_cnt_out[best_c], 1ULL);
  }
}

// ============================================================
// KMeans::run_cuda — 流式版
// ============================================================
KMeansResult KMeans::run_cuda(Dataset &data, u32 k) {
  KMeansResult result;
  result.k = k;
  u64 n = data.size();
  bool interleaved = (data.mode() == Dataset::Mode::Interleaved);
  std::printf("  [CUDA] 流式模式: %lu 点, K=%u, 交错=%s\n", n, k,
              interleaved ? "是" : "否");

  // 辅助取坐标
  const f64 *raw = nullptr;
  const f64 *xs = nullptr;
  const f64 *ys = nullptr;
  if (interleaved) {
    raw = data.raw_data();
  } else {
    xs = data.x();
    ys = data.y();
  }
  auto get_x = [=](u64 i) -> f64 { return interleaved ? raw[i * 2] : xs[i]; };
  auto get_y = [=](u64 i) -> f64 {
    return interleaved ? raw[i * 2 + 1] : ys[i];
  };
  // ---------- 分配设备内存（固定大小，与数据量无关） ----------
  f64 *d_x = nullptr, *d_y = nullptr;
  u32 *d_labels = nullptr;
  f64 *d_cx = nullptr, *d_cy = nullptr;
  f64 *d_sumx = nullptr, *d_sumy = nullptr;
  u64 *d_cnt = nullptr;
  u64 pts = kChunkPoints;
  if (!CUDA_OK(cudaMalloc(&d_x, pts * sizeof(f64))) ||
      !CUDA_OK(cudaMalloc(&d_y, pts * sizeof(f64))) ||
      !CUDA_OK(cudaMalloc(&d_labels, pts * sizeof(u32))) ||
      !CUDA_OK(cudaMalloc(&d_cx, k * sizeof(f64))) ||
      !CUDA_OK(cudaMalloc(&d_cy, k * sizeof(f64))) ||
      !CUDA_OK(cudaMalloc(&d_sumx, k * sizeof(f64))) ||
      !CUDA_OK(cudaMalloc(&d_sumy, k * sizeof(f64))) ||
      !CUDA_OK(cudaMalloc(&d_cnt, k * sizeof(u64)))) {
    // 清理
    // if (d_x) cudaFree(d_x); if (d_y) cudaFree(d_y);
    // if (d_labels) cudaFree(d_labels);
    // if (d_cx) cudaFree(d_cx); if (d_cy) cudaFree(d_cy);
    // if (d_sumx) cudaFree(d_sumx); if (d_sumy) cudaFree(d_sumy);
    // if (d_cnt) cudaFree(d_cnt);
    if (d_x) {
      cudaFree(d_x);
    }
    if (d_y) {
      cudaFree(d_y);
    }
    if (d_labels) {
      cudaFree(d_labels);
    }
    if (d_cx) {
      cudaFree(d_cx);
    }
    if (d_cy) {
      cudaFree(d_cy);
    }
    if (d_sumx) {
      cudaFree(d_sumx);
    }
    if (d_sumy) {
      cudaFree(d_sumy);
    }
    if (d_cnt) {
      cudaFree(d_cnt);
    }
    return result;
  }

  // ---------- 初始中心 ----------
  auto h_cx = std::make_unique<f64[]>(k);
  auto h_cy = std::make_unique<f64[]>(k);
  for (u32 j = 0; j < k; ++j) {
    h_cx[j] = get_x(j);
    h_cy[j] = get_y(j);
  }

  cudaMemcpy(d_cx, h_cx.get(), k * sizeof(f64), cudaMemcpyHostToDevice);
  cudaMemcpy(d_cy, h_cy.get(), k * sizeof(f64), cudaMemcpyHostToDevice);

  // ---------- 标签 ----------
  result.labels = std::make_unique<u32[]>(n);

  // ---------- CPU 归约缓存 ----------
  auto h_sumx = std::make_unique<f64[]>(k);
  auto h_sumy = std::make_unique<f64[]>(k);
  auto h_cnt = std::make_unique<u64[]>(k);

  std::printf("  [CUDA] Chunk: %llu 点, max %llu blocks\n",
              (unsigned long long)pts, (unsigned long long)((pts + 255) / 256));

  // ---------- 迭代 ----------
  for (i32 iter = 0; iter < config_.max_iterations; ++iter) {
    memset(h_sumx.get(), 0, k * sizeof(f64));
    memset(h_sumy.get(), 0, k * sizeof(f64));
    memset(h_cnt.get(), 0, k * sizeof(u64));
    u64 offset = 0;
    while (offset < n) {
      u64 chunk = std::min(pts, n - offset);
      // 构造 SoA chunk
      auto chunk_x = std::make_unique<f64[]>(chunk);
      auto chunk_y = std::make_unique<f64[]>(chunk);
      for (u64 i = 0; i < chunk; ++i) {
        chunk_x[i] = get_x(offset + i);
        chunk_y[i] = get_y(offset + i);
      }
      cudaMemcpy(d_x, chunk_x.get(), chunk * sizeof(f64),
                 cudaMemcpyHostToDevice);
      cudaMemcpy(d_y, chunk_y.get(), chunk * sizeof(f64),
                 cudaMemcpyHostToDevice);
      cudaMemset(d_sumx, 0, k * sizeof(f64));
      cudaMemset(d_sumy, 0, k * sizeof(f64));
      cudaMemset(d_cnt, 0, k * sizeof(u64));
      u32 blocks = (u32)((chunk + 255) / 256);
      u64 shmem = 2 * k * sizeof(f64);
      assign_kernel_stream<<<blocks, 256, shmem>>>(
          d_x, d_y, d_cx, d_cy, d_labels, d_sumx, d_sumy, d_cnt, chunk, k);
      cudaDeviceSynchronize();

      // 回传标签
      cudaMemcpy(result.labels.get() + offset, d_labels, chunk * sizeof(u32),
                 cudaMemcpyDeviceToHost);

      // 回传累加器
      auto csx = std::make_unique<f64[]>(k);
      auto csy = std::make_unique<f64[]>(k);
      auto cct = std::make_unique<u64[]>(k);
      cudaMemcpy(csx.get(), d_sumx, k * sizeof(f64), cudaMemcpyDeviceToHost);
      cudaMemcpy(csy.get(), d_sumy, k * sizeof(f64), cudaMemcpyDeviceToHost);
      cudaMemcpy(cct.get(), d_cnt, k * sizeof(u64), cudaMemcpyDeviceToHost);
      for (u32 j = 0; j < k; ++j) {
        h_sumx[j] += csx[j];
        h_sumy[j] += csy[j];
        h_cnt[j] += cct[j];
      }
      offset += chunk;
    }
    /**更新中心 */
    f64 delta = 0.0;
    for (u32 j = 0; j < k; ++j) {
      if (h_cnt[j] > 0) {
        f64 ncx = h_sumx[j] / (f64)h_cnt[j];
        f64 ncy = h_sumy[j] / (f64)h_cnt[j];
        delta += (ncx - h_cx[j]) * (ncx - h_cx[j]) +
                 (ncy - h_cy[j]) * (ncy - h_cy[j]);
        h_cx[j] = ncx;
        h_cy[j] = ncy;
      }
    }
    cudaMemcpy(d_cx, h_cx.get(), k * sizeof(f64), cudaMemcpyHostToDevice);
    cudaMemcpy(d_cy, h_cy.get(), k * sizeof(f64), cudaMemcpyHostToDevice);
    if ((iter + 1) % 5 == 0 || iter == 0) {
      std::printf("  [CUDA] Iter %3d / %d  delta=%.6e\n", iter + 1,
                  config_.max_iterations, delta);
    }
    if (delta < config_.threshold) {
      std::printf("  [CUDA] 收敛于迭代 %d (delta=%.6e)\n", iter, delta);
      result.iterations = iter + 1;
      break;
    }
  }
  if (result.iterations == 0) {
    result.iterations = config_.max_iterations;
  }

  // ---------- SSE ----------
  f64 inertia = 0.0;
#pragma omp parallel for reduction(+ : inertia) schedule(static)
  for (u64 i = 0; i < n; ++i) {
    u32 c = result.labels[i];
    f64 dx = get_x(i) - h_cx[c];
    f64 dy = get_y(i) - h_cy[c];
    inertia += dx * dx + dy * dy;
  }
  result.inertia = inertia;
  result.centers_x = std::move(h_cx);
  result.centers_y = std::move(h_cy);
  std::printf("  [CUDA] SSE = %.6e\n", inertia);
  cudaFree(d_x);
  cudaFree(d_y);
  cudaFree(d_labels);
  cudaFree(d_cx);
  cudaFree(d_cy);
  cudaFree(d_sumx);
  cudaFree(d_sumy);
  cudaFree(d_cnt);
  return result;
}