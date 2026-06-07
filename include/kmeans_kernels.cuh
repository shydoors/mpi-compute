#ifndef KMEANS_KERNELS_CUH
#define KMEANS_KERNELS_CUH

/**
 * @file kmeans_kernels.cuh
 * @brief CUDA kernel 声明
 *
 * 所有 kernel 使用 SoA 布局（separate x/y arrays），
 * 对齐至 128 字节（cache line），利用共享内存和向量化加载。
 */

#include "types.hpp"

#include <cstdint>

// ----------------------------------------------------------
// Kernel: 分配 + 局部聚合
//
// 每个 block 处理一个 tile 的点。
// blockDim.x 个线程协作：
//   1. 将 K 个中心加载到共享内存
//   2. 对 tile 中的每个点，计算到所有中心的距离（向量化）
//   3. 找到最近簇，写入标签
//   4. 局部累加 sumx, sumy, count 到共享内存
//
// 最终将局部累加结果写回全局内存的 per-block 累加器
// ----------------------------------------------------------

/// GPU 端每 block 的局部累加器大小
constexpr u32 kSharedAccumSize = 4'096;  // 最多支持 4096 个簇

/**
 * @brief CUDA kernel — 分配阶段
 *
 * 每个 block 分配一个点块，计算到所有中心的距离，
 * 写入标签并局部聚合。
 *
 * @param d_x        输入点 x 坐标（设备）
 * @param d_y        输入点 y 坐标（设备）
 * @param d_cx       中心点 x（设备）
 * @param d_cy       中心点 y（设备）
 * @param d_labels   输出标签（设备）
 * @param d_sumx_out 输出局部 sumx（设备, shape [num_blocks, K]）
 * @param d_sumy_out 输出局部 sumy（设备, shape [num_blocks, K]）
 * @param d_cnt_out  输出局部计数（设备, shape [num_blocks, K]）
 * @param n          总点数
 * @param k          簇数
 * @param tile_size  每个 block 处理的点数
 */
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
    u32 tile_size
);

// ----------------------------------------------------------
// Kernel: 归约 — 合并所有 block 的局部累加结果，更新中心
// ----------------------------------------------------------

/**
 * @brief CUDA kernel — 归约阶段
 *
 * 将 per-block 的局部累加器归约，计算新中心及 delta。
 *
 * @param d_sumx_in   每个 block 的局部 sumx
 * @param d_sumy_in   每个 block 的局部 sumy
 * @param d_cnt_in    每个 block 的局部计数
 * @param d_cx        旧中心 x（输入），新中心 x（输出）
 * @param d_cy        旧中心 y（输入），新中心 y（输出）
 * @param d_delta_out 中心移动平方和（标量输出）
 * @param num_blocks  block 总数
 * @param k           簇数
 */
__global__ void reduce_kernel(
    const f64* __restrict__ d_sumx_in,
    const f64* __restrict__ d_sumy_in,
    const u64* __restrict__ d_cnt_in,
    f64*       __restrict__ d_cx,
    f64*       __restrict__ d_cy,
    f64*       __restrict__ d_delta_out,
    u32 num_blocks,
    u32 k
);

// ----------------------------------------------------------
// Kernel: 计算完整 Inertia（SSE）
// ----------------------------------------------------------

/**
 * @brief CUDA kernel — 计算 SSE
 *
 * @param d_x        输入点 x（设备）
 * @param d_y        输入点 y（设备）
 * @param d_cx       中心 x（设备）
 * @param d_cy       中心 y（设备）
 * @param d_labels   簇标签（设备）
 * @param d_inertia_out 输出 inertia（标量，设备）
 * @param n          总点数
 * @param k          簇数
 */
__global__ void inertia_kernel(
    const f64* __restrict__ d_x,
    const f64* __restrict__ d_y,
    const f64* __restrict__ d_cx,
    const f64* __restrict__ d_cy,
    const u32* __restrict__ d_labels,
    f64*       __restrict__ d_inertia_out,
    u64 n,
    u32 k
);

#endif  // KMEANS_KERNELS_CUH
