# 项目设计文档

## 概述

本项目实现 K-Means 聚类算法，针对 **9+ GiB 超大规模数据集**（`double (x,y)` 坐标点）进行优化。
采用 **CUDA / OpenMP / 串行** 多后端并行加速，支持**自动 K 推导**（Kneedle 肘部检测）和**断点继续**。

所有配置通过 `src/config.json` 管理，无需重新编译。

---

## 1. K-Means 算法

### 1.1 定义

K‑Means 是一种基于划分的聚类算法，也是最经典的无监督学习方法之一。

**核心目标**：将 `n` 个样本划分到 `K` 个簇中，使得簇内样本尽可能紧凑，簇间样本尽可能分离。

**数学上**，它最小化所有样本到其所属簇中心的**距离平方和**（称为 **Inertia** 或 **簇内误差平方和 SSE**）：

```
SSE = Σ ||x_i - μ_c(i)||²
```

其中 $x_i$ 是第 $i$ 个样本点，$c(i)$ 是其所属簇，$\mu_c$ 是簇 $c$ 的中心。

### 1.2 算法流程（标准 Lloyd）

```
1. 初始化 K 个中心（K-Means++ 或前 K 个点）
2. 重复直到收敛：
   a. 分配：对每个点分配到最近的中心
   b. 更新：重新计算每个簇的中心
   c. 如果中心移动量 < 阈值，则收敛
```

---

## 2. 并行架构设计

### 2.1 后端对比

| 后端 | 适用场景 | 加速比 |
|------|----------|--------|
| **Sequential** | 验证正确性，小数据集 | 1x |
| **OpenMP** | CPU 多核，内存充足 | ~30x |
| **CUDA 流式** | GPU 加速，超大/超小数据集 | ~100x |

### 2.2 Fork-Join MapReduce 模型

每轮迭代：
- **Fork**: 将 N 个点切分为 T 块
- **Map**: 每块独立计算点到 K 个中心的距离，分配最近簇，局部累加
- **Join**: 归约所有块的累加器，更新中心，计算 delta

### 2.3 CUDA 流式设计

对于 9+ GiB 无法放入显存的数据集，采用分块流式处理：

```
每轮迭代:
  循环读取数据块 (chunk = 64 MB):
    1. 上传 chunk 到 GPU
    2. GPU kernel 计算分配 + 局部聚合
    3. 回传标签和局部累加器
    4. CPU 归约
  更新中心，上传到 GPU，进入下一轮
```

GPU 显存只需容纳一个 chunk（~64 MB）+ 中心数组，与总数据量无关。

---

## 3. 自动 K 推导

### 3.1 流程

```
1. 均匀采样（默认 5 万点）
2. 对样本运行 K-Means, K = 2 .. min(√N, 4000)
3. 记录每个 K 的 SSE
4. Kneedle 算法定位肘点
5. 输出最优 K
```

### 3.2 Kneedle 算法

找到 SSE 曲线中曲率最大的点（拐点）：

```
1. 归一化 x, y 到 [0, 1]
2. 计算每个点到对角线 (0,1)→(1,0) 的距离
3. 距离最大的点即为肘点
```

---

## 4. 数据加载

### 4.1 文件格式

纯二进制 `double[2]` 序列（小端序），每点 16 字节。

### 4.2 加载策略

| 策略 | 条件 | 内存占用 | 说明 |
|------|------|----------|------|
| **mmap + SoA 分离** | 文件 < 1 GiB | ~2x | 兼容所有后端 |
| **mmap 交错零拷贝** | 文件 ≥ 1 GiB | ~1x | 大文件默认策略 |
| **流式分块** | 手动启用 | ~chunk_size | 内存极度受限时 |

### 4.3 SoA 分离 vs 交错零拷贝

**SoA（小文件）**:
```
x[]: x₀ x₁ x₂ ... x_{n-1}
y[]: y₀ y₁ y₂ ... y_{n-1}
```
利于 SIMD 批量加载、GPU 合并访问。

**交错零拷贝（大文件）**:
```
raw[]: x₀ y₀ x₁ y₁ x₂ y₂ ...
```
直接访问 mmap 映射，不复制数据，省去 SoA 转换的 ~2x 内存。

---

## 5. CUDA GPU 加速

### 5.1 流式 Kernel

| Kernel | 功能 | 说明 |
|--------|------|------|
| `assign_kernel_stream` | 分配 + 局部聚合 | 每 block 处理一个 tile，中心放在共享内存 |

### 5.2 共享内存使用

- 中心副本：$k \times 2 \times 8$ 字节
- 无归约暂存（直接 atomicAdd 到全局）

### 5.3 优化策略

- 中心加载到共享内存
- 循环展开（`#pragma unroll 4`）
- `atomicAdd` 写累加器
- `__restrict__` 指针别名提示

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

在 `src/config.json` 中设置 `checkpoint_path` 和 `resume: true`。

---

## 7. 项目结构

```
mpi-compute/
├── CMakeLists.txt              # 根构建文件
├── src/                        # 源文件
│   ├── config.json             # 运行配置
│   ├── main.cpp                # 主入口
│   ├── kmeans.cpp              # K-Means 主类 + 自动 K
│   ├── kmeans_sequential.cpp   # CPU 串行后端
│   ├── kmeans_omp.cpp          # OpenMP 并行后端
│   ├── kmeans_kernels.cu       # CUDA 后端（流式）
│   ├── dataset.cpp             # 数据集加载
│   └── checkpoint.cpp          # 断点继续
├── include/                    # 头文件
│   ├── types.hpp
│   ├── dataset.hpp
│   ├── kmeans.hpp
│   ├── kmeans_kernels.cuh
│   └── checkpoint.hpp
├── code-stu/                   # 学习代码、课本示例
├── script/                     # 工具脚本
│   ├── render.py
│   └── dat_out.cpp
├── data/                       # 输入数据
├── results/                    # 运行结果
├── docs/                       # 文档
└── build/                      # 构建产物
```

---

## 8. 构建与运行

### 8.1 构建

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 8.2 运行

修改 `src/config.json` 中的参数，然后运行：

```bash
./build/kmeans
```

所有配置通过 JSON 文件管理，无 CLI 参数。
