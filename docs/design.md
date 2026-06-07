# 项目设计文档

## 概述

本项目实现 K-Means 聚类算法，针对 **9+ GiB 超大规模数据集**（`double (x,y)` 坐标点，500~4000 个簇）。
采用 **CUDA + OpenMP + MPI** 多后端并行加速，支持**自动 K 推导**（Kneedle 肘部检测）和**断点继续**。

---

## 1. K-Means 算法

### 1.1 定义

K‑Means 是一种基于划分的聚类算法，也是最经典的无监督学习方法之一。

**核心目标**：将 `n` 个样本划分到 `K` 个簇中，使得**簇内样本尽可能相似（紧凑），簇间样本尽可能不同（分离）**。

**数学上**，它最小化所有样本到其所属簇中心的**距离平方和**（称为 **Inertia** 或 **簇内误差平方和 SSE**）：

```
SSE = Σᵢ ||xᵢ - μ_{c(i)}||²
```

其中：
- `xᵢ` — 第 i 个样本点
- `c(i)` — 点 xᵢ 所属的簇
- `μ_{c}` — 簇 c 的中心

### 1.2 算法流程（标准 Lloyd）

```
1. 初始化 K 个中心 μ₁, μ₂, ..., μₖ
2. 重复直到收敛：
   a. 分配：对每个点 xᵢ，分配到最近的中心
      c(i) = argminⱼ ||xᵢ - μⱼ||²
   b. 更新：重新计算每个簇的中心
      μⱼ = (1/|Cⱼ|) * Σ_{i∈Cⱼ} xᵢ
   c. 如果中心移动量 < 阈值，则收敛
```

---

## 2. 并行架构设计

### 2.1 并行模型选型

| 模型 | 应用位置 | 说明 |
|------|----------|------|
| **Fork-Join (MapReduce)** | 每轮迭代 | 点分配 = Map，归约聚合 = Reduce |
| **SIMT (CUDA)** | GPU 距离计算 | 每个线程处理一个点，warp 级归约 |
| **SIMD (CPU自动向量化)** | CPU 距离计算 | 编译器自动向量化循环 |
| **生产者-消费者** | 流式 I/O | I/O 线程生产数据块，计算线程消费 |

### 2.2 三层并行

```
┌─────────────────────────────────────────┐
│  MPI 多节点（可选）                       │
│  每个节点处理一部分数据，MPI_Allreduce    │
├─────────────────────────────────────────┤
│  OpenMP / CUDA 单节点并行                │
│  Fork-Join: 数据分块 → 映射 → 归约       │
├─────────────────────────────────────────┤
│  指令级并行（SIMD/SIMT）                 │
│  距离计算循环自动向量化 / GPU warp        │
└─────────────────────────────────────────┘
```

### 2.3 迭代流程（Fork-Join MapReduce）

```
每轮迭代:
  [Fork]  将 N 个点切分为 T 块（T = 线程/block数）
  [Map]   每块独立:
             1. 计算点到 K 个中心的距离
             2. 分配最近簇
             3. 局部累加 sumx, sumy, count
  [Join]  全局归约:
             1. 合并所有块的局部累加器
             2. 计算新中心
             3. 计算中心移动量 delta
  [收敛]  若 delta < threshold，停止
```

---

## 3. 自动 K 推导

### 3.1 流程

```
1. 均匀采样 ~1M 点（固定步长）
2. 对样本运行 Lloyd K-Means, K = 2..min(√N, 4000)
3. 记录每个 K 的 SSE
4. Kneedle 算法定位肘点
5. 输出最优 K
```

### 3.2 Kneedle 算法

原理：找到 SSE 曲线中曲率最大的点（拐点）。

```
1. 归一化 x, y 到 [0, 1]
2. 计算每个点到对角线 (0,1)→(1,0) 的距离
3. 距离最大的点即为肘点
```

---

## 4. 数据加载

### 4.1 文件格式

纯二进制 `double[2]` 序列（小端序），每点 16 字节。

```
| x (f64, 8 bytes) | y (f64, 8 bytes) |
| x (f64, 8 bytes) | y (f64, 8 bytes) |
| ...                                    |
```

### 4.2 加载策略

| 策略 | 适用场景 | 说明 |
|------|----------|------|
| **mmap** | 内存充足（≥16 GiB） | 零拷贝，按需分页 |
| **read** | 内存一般 | 逐点读取，分离 SoA |
| **流式分块** | 内存紧张 | 生产者-消费者，I/O 与计算重叠 |

### 4.3 内存布局

采用 **SoA（Structure of Arrays）** 分离存储：

```
x[]: | x₀ | x₁ | x₂ | ... | x_{n-1} |
y[]: | y₀ | y₁ | y₂ | ... | y_{n-1} |
```

优势：利于 SIMD 批量加载、GPU 合并访问、缓存友好。

---

## 5. CUDA GPU 加速

### 5.1 Kernel 设计

| Kernel | 功能 | Grid/Block |
|--------|------|------------|
| `assign_kernel` | 分配+局部聚合 | grid=(N/tile), block=256 |
| `reduce_kernel` | 归约所有 block，更新中心 | grid=1, block=256 |
| `inertia_kernel` | 计算 SSE | grid~256, block=256 |

### 5.2 共享内存使用

每个 block 动态分配共享内存：
- 中心点副本：`k * 2 * sizeof(f64)` 字节
- 归约暂存：`blockDim.x * (sizeof(f64) + sizeof(u32))` 字节

### 5.3 优化策略

- 中心加载到共享内存（避免重复全局读取）
- 循环展开（`#pragma unroll 4`）
- atomicAdd 写累加器（冲突概率低）
- `__restrict__` 指针别名提示
- `--use_fast_math` 编译优化

---

## 6. 断点继续（Checkpoint）

### 6.1 格式

```
[Header: 32 bytes]
  - magic:    u64 = 0x4B4D45414E535F43  ("KMEANS_C")
  - version:  u32 = 1
  - k:        u32
  - iteration: i32
  - reserved: u64 = 0
[Centers: 16*k bytes]
  - cx[0..k-1]: f64
  - cy[0..k-1]: f64
```

### 6.2 使用方式

```bash
# 启动时自动保存 checkpoint（每 5 轮）
./kmeans --data data/b.dat --checkpoint ckpt.bin

# 从中断处恢复
./kmeans --data data/b.dat --checkpoint ckpt.bin --resume
```

---

## 7. 项目结构

```
mpi-compute/
├── CMakeLists.txt              # 根构建（C++17 + CUDA + OpenMP）
├── include/                    # 头文件
│   ├── types.hpp               # 类型定义
│   ├── dataset.hpp             # 数据集管理
│   ├── kmeans.hpp              # K-Means 主接口
│   ├── kmeans_kernels.cuh      # CUDA kernel 声明
│   └── checkpoint.hpp          # 断点继续
├── src/                        # 源文件
│   ├── main.cpp                # 主入口 + CLI 解析
│   ├── dataset.cpp             # 数据集实现
│   ├── kmeans.cpp              # K-Means 主类 + 自动 K
│   ├── kmeans_sequential.cpp   # CPU 串行后端（验证）
│   ├── kmeans_omp.cpp          # OpenMP 并行后端
│   ├── kmeans_kernels.cu       # CUDA 后端
│   └── checkpoint.cpp          # 断点继续实现
├── data/                       # 输入/输出数据
├── script/                     # 工具脚本
│   ├── CMakeLists.txt
│   └── dat_out.cpp             # 数据预览
├── docs/                       # 文档
│   ├── design.md
│   ├── parallelism.md
│   └── requirements.md
├── build/                      # 构建产物
└── README.md
```

---

## 8. 构建与运行

### 8.1 构建

```bash
mkdir -p build && cd build

# Release 模式（默认）
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Debug 模式
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### 8.2 运行

```bash
# CUDA 后端（默认）
./build/kmeans --data data/b.dat

# 自动 K 推导
./build/kmeans --data data/b.dat --auto-k 1

# 手动指定 K
./build/kmeans --data data/b.dat --auto-k 0 --k 500

# CPU OpenMP 后端
./build/kmeans --data data/b.dat --backend omp

# 流式读取（内存不足时）
./build/kmeans --data data/b.dat --streaming

# 断点继续
./build/kmeans --data data/b.dat --checkpoint ckpt.bin --resume

# 所有选项
./build/kmeans --help
```
