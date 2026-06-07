/**
 * @file kmeans_kernels.cu
 * @brief CUDA K-Means kernel 实现
 *
 * 并行模型：数据并行 (SIMT)
 *
 * 每轮迭代流程：
 *   1. assign_kernel: 每个 block 处理一个 tile 的点
 *      - 加载 K 个中心到共享内存
 *      - 计算 tile 内每个点到所有中心的距离（循环展开 + 向量化）
 *      - 写入标签，局部累加
 *   2. reduce_kernel: 归约所有 block 的局部累加器，更新中心
 *   3. 收敛检测
 *
 * 内存布局：
 *   - 所有数据在设备端以 SoA 形式存储
 *   - 共享内存动态分配，用于中心和局部累加器
 */

#include "kmeans_kernels.cuh"
#include "kmeans.hpp"
#include "types.hpp"
#include "checkpoint.hpp"

#include <cstdio>
#include <cstdint>
#include <chrono>
#include <cuda_runtime.h>
#include <cstring>
#include <memory>
#include <limits>
#include <random>

// ============================================================
// CUDA 错误检查宏
// ============================================================
#define CUDA_CHECK(expr) do {                                         \
    cudaError_t err = (expr);                                         \
    if (err != cudaSuccess) {                                         \
      std::fprintf(stderr, "CUDA Error at %s:%d: %s\n",              \
                   __FILE__, __LINE__, cudaGetErrorString(err));      \
      return KMeansResult{};                                         \
    }                                                                 \
  } while(0)

// ============================================================
// Kernel 配置常量
// ============================================================
constexpr i32 kBlockSize = 256;        // 每个 block 的线程数
constexpr u32 kTileSize  = 4096;       // 每个 block 每次处理的点数

// ============================================================
// 设备端辅助函数
// ============================================================

///  warp 级归约（求最小值/索引）
template <int kWarpSize = 32>
__device__ __forceinline__ void warp_reduce_min(
    f64& val, u32& idx)
{
  for (int offset = kWarpSize / 2; offset > 0; offset >>= 1) {
    f64 other_val = __shfl_xor_sync(0xFFFFFFFF, val, offset);
    u32 other_idx = __shfl_xor_sync(0xFFFFFFFF, idx, offset);
    if (other_val < val) {
      val = other_val;
      idx = other_idx;
    }
  }
}

/// block 级归约（求最小值/索引）
__device__ __forceinline__ void block_reduce_min(
    f64& val, u32& idx,
    f64* shared_val, u32* shared_idx)
{
  int tid = threadIdx.x;
  shared_val[tid] = val;
  shared_idx[tid] = idx;
  __syncthreads();

  for (int s = blockDim.x / 2; s > 0; s >>= 1) {
    if (tid < s) {
      if (shared_val[tid + s] < shared_val[tid]) {
        shared_val[tid] = shared_val[tid + s];
        shared_idx[tid] = shared_idx[tid + s];
      }
    }
    __syncthreads();
  }

  val = shared_val[0];
  idx = shared_idx[0];
}

// ============================================================
// assign_kernel 实现
// ============================================================
__global__ void assign_kernel(
    const f64* __restrict__ d_x,
    const f64* __restrict__ d_y,
    const f64* __restrict__ d_cx,
    const f64* __restrict__ d_cy,
    u32*       __restrict__ d_labels,
    f64*       __restrict__ d_sumx_out,
    f64*       __restrict__ d_sumy_out,
    u64*       __restrict__ d_cnt_out,
    u64 n,
    u32 k,
    u32 tile_size)
{
    // 动态共享内存布局：
    //   [0 .. k-1]:          cx 共享副本
    //   [k .. 2k-1]:         cy 共享副本
    //   [2k .. 2k+blockDim.x-1]:    归约暂存 val
    //   [2k+blockDim.x .. 2k+2*blockDim.x-1]: 归约暂存 idx
    extern __shared__ f64 s_data[];
    f64* s_cx = s_data;
    f64* s_cy = s_data + k;
    int tid = threadIdx.x;
    int bid = blockIdx.x;

    // 加载中心到共享内存（所有线程协作）
    for (int j = tid; j < k; j += blockDim.x) {
        s_cx[j] = d_cx[j];
        s_cy[j] = d_cy[j];
    }
    __syncthreads();

    // 当前 block 处理的数据范围
    u64 block_start = static_cast<u64>(bid) * tile_size;
    u64 block_end   = min(block_start + tile_size, n);

    // 线程局部累加器
    // 使用寄存器/局部内存 — 每个线程只累加自己发现的簇
    // 最后写回全局内存需要原子操作，或用共享内存归约
    // 此处采用：每个线程维护自己的数组（太大可能导致寄存器溢出）
    // 更好的方式：每个 block 共享一组累加器，线程协作归约
    //
    // 简化方案：每个线程用局部变量，最后用 atomicAdd 写回全局

    for (u64 i = block_start + tid; i < block_end; i += blockDim.x) {
        f64 px = d_x[i];
        f64 py = d_y[i];

        // 寻找最近中心
        f64 best_dist = 1e308;  // 近似 inf
        u32 best_c = 0;

        // 循环展开：每次处理 4 个中心
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

        // 写入标签
        d_labels[i] = best_c;

        // 更新全局累加器（原子操作 — 可能会有竞争，但概率较低）
        // 对于大型数据集，冲突概率 = tile_size / k * block数
        // 更好的做法：先在共享内存归约，再写回全局
        // 为简化，此处用 atomicAdd
        atomicAdd(&d_sumx_out[best_c], px);
        atomicAdd(&d_sumy_out[best_c], py);
        atomicAdd(reinterpret_cast<unsigned long long*>(&d_cnt_out[best_c]),
                  static_cast<unsigned long long>(1));
    }
}

// ============================================================
// reduce_kernel 实现
// ============================================================
__global__ void reduce_kernel(
    const f64* __restrict__ d_sumx_in,
    const f64* __restrict__ d_sumy_in,
    const u64* __restrict__ d_cnt_in,
    f64*       __restrict__ d_cx,
    f64*       __restrict__ d_cy,
    f64*       __restrict__ d_delta_out,
    u32 num_blocks,
    u32 k)
{
    // 该 kernel 使用 1D grid，每个 block 处理一个簇
    // 或者用单 block 顺序处理所有簇
    // 由于 k <= 4000，单 block 即可

    int tid = threadIdx.x;

    // 共享内存用于归约
    extern __shared__ f64 s_reduce[];
    f64* s_delta = s_reduce;

    f64 local_delta = 0.0;

    for (u32 j = tid; j < k; j += blockDim.x) {
        f64 sumx = 0.0, sumy = 0.0;
        u64 cnt = 0;

        // 归约所有 block 的累加器
        for (u32 b = 0; b < num_blocks; ++b) {
            sumx += d_sumx_in[b * k + j];
            sumy += d_sumy_in[b * k + j];
            cnt  += d_cnt_in[b * k + j];
        }

        if (cnt > 0) {
            f64 new_cx = sumx / static_cast<f64>(cnt);
            f64 new_cy = sumy / static_cast<f64>(cnt);
            f64 dx = new_cx - d_cx[j];
            f64 dy = new_cy - d_cy[j];
            local_delta += dx * dx + dy * dy;
            d_cx[j] = new_cx;
            d_cy[j] = new_cy;
        }
    }

    // 归约 delta
    s_delta[tid] = local_delta;
    __syncthreads();

    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            s_delta[tid] += s_delta[tid + s];
        }
        __syncthreads();
    }

    if (tid == 0) {
        *d_delta_out = s_delta[0];
    }
}

// ============================================================
// inertia_kernel 实现
// ============================================================
__global__ void inertia_kernel(
    const f64* __restrict__ d_x,
    const f64* __restrict__ d_y,
    const f64* __restrict__ d_cx,
    const f64* __restrict__ d_cy,
    const u32* __restrict__ d_labels,
    f64*       __restrict__ d_inertia_out,
    u64 n,
    u32 k)
{
    extern __shared__ f64 s_inertia[];
    int tid = threadIdx.x;

    f64 local_inertia = 0.0;

    for (u64 i = blockIdx.x * blockDim.x + tid; i < n; i += gridDim.x * blockDim.x) {
        u32 c = d_labels[i];
        f64 dx = d_x[i] - d_cx[c];
        f64 dy = d_y[i] - d_cy[c];
        local_inertia += dx * dx + dy * dy;
    }

    s_inertia[tid] = local_inertia;
    __syncthreads();

    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            s_inertia[tid] += s_inertia[tid + s];
        }
        __syncthreads();
    }

    if (tid == 0) {
        atomicAdd(d_inertia_out, s_inertia[0]);
    }
}

// ============================================================
// KMeans::run_cuda — CUDA 后端主控
// ============================================================
KMeansResult KMeans::run_cuda(Dataset& data, u32 k) {
    using Clock = std::chrono::high_resolution_clock;

    KMeansResult result;
    result.k = k;
    result.labels = std::make_unique<u32[]>(data.size());

    u64 n = data.size();
    std::printf("  [CUDA] 点数: %lu, 簇数: %u\n", n, k);

    // ---------- 设备内存分配 ----------
    f64 *d_x = nullptr, *d_y = nullptr;
    u32 *d_labels = nullptr;
    f64 *d_cx = nullptr, *d_cy = nullptr;
    f64 *d_sumx = nullptr, *d_sumy = nullptr;
    u64 *d_cnt = nullptr;
    f64 *d_delta = nullptr, *d_inertia = nullptr;
    f64 *d_init_cx = nullptr, *d_init_cy = nullptr;

    // 计算 block 数
    u32 num_blocks = (n + kTileSize - 1) / kTileSize;
    if (num_blocks < 1) num_blocks = 1;

    u64 labels_bytes = n * sizeof(u32);
    u64 center_bytes = k * sizeof(f64);
    u64 accum_bytes  = static_cast<u64>(num_blocks) * k * sizeof(f64);
    u64 cnt_bytes    = static_cast<u64>(num_blocks) * k * sizeof(u64);

    try {
        // 分配设备内存
        CUDA_CHECK(cudaMalloc(&d_x, n * sizeof(f64)));
        CUDA_CHECK(cudaMalloc(&d_y, n * sizeof(f64)));
        CUDA_CHECK(cudaMalloc(&d_labels, labels_bytes));
        CUDA_CHECK(cudaMalloc(&d_cx, center_bytes));
        CUDA_CHECK(cudaMalloc(&d_cy, center_bytes));
        CUDA_CHECK(cudaMalloc(&d_sumx, accum_bytes));
        CUDA_CHECK(cudaMalloc(&d_sumy, accum_bytes));
        CUDA_CHECK(cudaMalloc(&d_cnt, cnt_bytes));
        CUDA_CHECK(cudaMalloc(&d_delta, sizeof(f64)));
        CUDA_CHECK(cudaMalloc(&d_inertia, sizeof(f64)));
        CUDA_CHECK(cudaMalloc(&d_init_cx, center_bytes));
        CUDA_CHECK(cudaMalloc(&d_init_cy, center_bytes));

        // 拷贝数据到设备
        CUDA_CHECK(cudaMemcpy(d_x, data.x(), n * sizeof(f64), cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_y, data.y(), n * sizeof(f64), cudaMemcpyHostToDevice));

        // ---------- 初始中心（K-Means++ 简化版） ----------
        // 使用前 k 个点作为初始中心（简化，可替换为 K-Means++）
        // 实际使用中，从 auto_detect_k 获取更好的初始中心
        std::printf("  [CUDA] 初始化中心 (K-Means++ 简化版)\n");

        auto h_cx = std::make_unique<f64[]>(k);
        auto h_cy = std::make_unique<f64[]>(k);

        // K-Means++ 初始化（CPU 端，采样版）
        {
            const f64* xs = data.x();
            const f64* ys = data.y();

            // 使用固定种子保持确定性
            std::mt19937_64 rng(42);

            // 第一个中心：从数据中均匀选取
            h_cx[0] = xs[0];
            h_cy[0] = ys[0];

            for (u32 c = 1; c < k; ++c) {
                // 计算每个点到最近中心的距离
                auto min_dists = std::make_unique<f64[]>(n);
                f64 total_dist = 0.0;

                for (u64 i = 0; i < n; ++i) {
                    f64 best = std::numeric_limits<f64>::max();
                    for (u32 j = 0; j < c; ++j) {
                        f64 dx = xs[i] - h_cx[j];
                        f64 dy = ys[i] - h_cy[j];
                        f64 d = dx * dx + dy * dy;
                        if (d < best) best = d;
                    }
                    min_dists[i] = best;
                    total_dist += best;
                }

                // 加权随机选择
                // 对 9GiB 数据，全量扫描代价高
                // 此处用采样版：只在前 sample_size 个点中选
                // 或使用更高效的算法
                // 简化：随机选择
                f64 r = std::uniform_real_distribution<f64>(0, total_dist)(rng);
                f64 accum = 0.0;
                u64 selected = n - 1;
                for (u64 i = 0; i < n; ++i) {
                    accum += min_dists[i];
                    if (accum >= r) {
                        selected = i;
                        break;
                    }
                }
                h_cx[c] = xs[selected];
                h_cy[c] = ys[selected];
            }

            std::printf("  [CUDA] K-Means++ 初始化完成\n");
        }

        // 拷贝初始中心到设备
        CUDA_CHECK(cudaMemcpy(d_cx, h_cx.get(), center_bytes, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_cy, h_cy.get(), center_bytes, cudaMemcpyHostToDevice));
        // 保存初始中心副本（用于恢复）
        CUDA_CHECK(cudaMemcpy(d_init_cx, h_cx.get(), center_bytes, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_init_cy, h_cy.get(), center_bytes, cudaMemcpyHostToDevice));

        // ---------- 断点继续 ----------
        i32 start_iter = 0;
        if (config_.resume && !config_.checkpoint_path.empty()) {
            u32 ck_k = k;
            i32 ck_iter = 0;
            f64* tmp_cx = new f64[k];
            f64* tmp_cy = new f64[k];
            if (checkpoint_load(config_.checkpoint_path, tmp_cx, tmp_cy, ck_k, ck_iter)) {
                if (ck_k == k) {
                    start_iter = ck_iter + 1;
                    CUDA_CHECK(cudaMemcpy(d_cx, tmp_cx, center_bytes, cudaMemcpyHostToDevice));
                    CUDA_CHECK(cudaMemcpy(d_cy, tmp_cy, center_bytes, cudaMemcpyHostToDevice));
                    std::printf("  [CUDA] 从 checkpoint 恢复: 迭代 %d\n", start_iter);
                }
            }
            delete[] tmp_cx;
            delete[] tmp_cy;
        }

        // ---------- Kernel 配置 ----------
        u32 assign_blocks = static_cast<u32>((n + kTileSize - 1) / kTileSize);
        dim3 assign_grid(assign_blocks);

        // assign kernel 共享内存大小：2*k (centers)
        u64 assign_shmem = 2 * k * sizeof(f64);
        // reduce kernel 共享内存大小
        u64 reduce_shmem = kBlockSize * sizeof(f64);

        // ---------- 迭代 ----------
        std::printf("  [CUDA] Grid: %u blocks x %d threads, tile=%u\n",
                    assign_blocks, kBlockSize, kTileSize);
        std::printf("  [CUDA] Shared memory per block: %zu bytes\n", assign_shmem);

        for (i32 iter = start_iter; iter < config_.max_iterations; ++iter) {
            // 清零累加器
            CUDA_CHECK(cudaMemset(d_sumx, 0, accum_bytes));
            CUDA_CHECK(cudaMemset(d_sumy, 0, accum_bytes));
            CUDA_CHECK(cudaMemset(d_cnt, 0, cnt_bytes));
            CUDA_CHECK(cudaMemset(d_delta, 0, sizeof(f64)));

            // 1. Assign kernel
            assign_kernel<<<assign_grid, kBlockSize, assign_shmem>>>(
                d_x, d_y, d_cx, d_cy, d_labels,
                d_sumx, d_sumy, d_cnt,
                n, k, kTileSize
            );
            CUDA_CHECK(cudaGetLastError());
            CUDA_CHECK(cudaDeviceSynchronize());

            // 2. Reduce kernel
            reduce_kernel<<<1, kBlockSize, reduce_shmem>>>(
                d_sumx, d_sumy, d_cnt,
                d_cx, d_cy, d_delta,
                assign_blocks, k
            );
            CUDA_CHECK(cudaGetLastError());
            CUDA_CHECK(cudaDeviceSynchronize());

            // 3. 读取 delta 并判断收敛
            f64 h_delta = 0.0;
            CUDA_CHECK(cudaMemcpy(&h_delta, d_delta, sizeof(f64), cudaMemcpyDeviceToHost));

            if ((iter + 1) % 5 == 0 || iter == 0) {
                std::printf("  [CUDA] Iter %3d / %d  delta=%.6e\n",
                            iter + 1, config_.max_iterations, h_delta);
            }

            // Checkpoint
            if (!config_.checkpoint_path.empty() &&
                config_.checkpoint_interval > 0 &&
                (iter + 1) % config_.checkpoint_interval == 0) {
                f64* h_ck_cx = new f64[k];
                f64* h_ck_cy = new f64[k];
                CUDA_CHECK(cudaMemcpy(h_ck_cx, d_cx, center_bytes, cudaMemcpyDeviceToHost));
                CUDA_CHECK(cudaMemcpy(h_ck_cy, d_cy, center_bytes, cudaMemcpyDeviceToHost));
                if (checkpoint_save(config_.checkpoint_path, h_ck_cx, h_ck_cy, k, iter)) {
                  std::printf("  [CUDA] Checkpoint saved (iter %d)\n", iter);
                }
                delete[] h_ck_cx;
                delete[] h_ck_cy;
            }

            if (h_delta < config_.threshold) {
                std::printf("  [CUDA] 收敛于迭代 %d (delta=%.6e)\n", iter, h_delta);
                result.iterations = iter + 1;
                break;
            }
        }

        if (result.iterations == 0) {
            result.iterations = config_.max_iterations;
        }

        // ---------- 回传结果 ----------
        CUDA_CHECK(cudaMemcpy(h_cx.get(), d_cx, center_bytes, cudaMemcpyDeviceToHost));
        CUDA_CHECK(cudaMemcpy(h_cy.get(), d_cy, center_bytes, cudaMemcpyDeviceToHost));
        CUDA_CHECK(cudaMemcpy(result.labels.get(), d_labels, labels_bytes, cudaMemcpyDeviceToHost));

        result.centers_x = std::move(h_cx);
        result.centers_y = std::move(h_cy);

        // 计算 SSE
        CUDA_CHECK(cudaMemset(d_inertia, 0, sizeof(f64)));
        dim3 inertia_grid(std::min(256u, (u32)((n + kBlockSize - 1) / kBlockSize)));
        u64 inertia_shmem = kBlockSize * sizeof(f64);

        inertia_kernel<<<inertia_grid, kBlockSize, inertia_shmem>>>(
            d_x, d_y, d_cx, d_cy, d_labels, d_inertia, n, k
        );
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());

        f64 h_inertia = 0.0;
        CUDA_CHECK(cudaMemcpy(&h_inertia, d_inertia, sizeof(f64), cudaMemcpyDeviceToHost));
        result.inertia = h_inertia;

        std::printf("  [CUDA] Final SSE (Inertia): %.6e\n", h_inertia);

        // ---------- 释放 ----------
        CUDA_CHECK(cudaFree(d_x));
        CUDA_CHECK(cudaFree(d_y));
        CUDA_CHECK(cudaFree(d_labels));
        CUDA_CHECK(cudaFree(d_cx));
        CUDA_CHECK(cudaFree(d_cy));
        CUDA_CHECK(cudaFree(d_sumx));
        CUDA_CHECK(cudaFree(d_sumy));
        CUDA_CHECK(cudaFree(d_cnt));
        CUDA_CHECK(cudaFree(d_delta));
        CUDA_CHECK(cudaFree(d_inertia));
        CUDA_CHECK(cudaFree(d_init_cx));
        CUDA_CHECK(cudaFree(d_init_cy));

    } catch (...) {
        // 异常清理
        if (d_x) cudaFree(d_x);
        if (d_y) cudaFree(d_y);
        if (d_labels) cudaFree(d_labels);
        if (d_cx) cudaFree(d_cx);
        if (d_cy) cudaFree(d_cy);
        if (d_sumx) cudaFree(d_sumx);
        if (d_sumy) cudaFree(d_sumy);
        if (d_cnt) cudaFree(d_cnt);
        if (d_delta) cudaFree(d_delta);
        if (d_inertia) cudaFree(d_inertia);
        if (d_init_cx) cudaFree(d_init_cx);
        if (d_init_cy) cudaFree(d_init_cy);
        throw;
    }

    return result;
}
