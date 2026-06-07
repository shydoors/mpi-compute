# mpi-compute — K-Means 聚类算法（教学项目）

本项目实现了 **K-Means 聚类算法**，支持 **CUDA / OpenMP / 串行** 多后端并行加速，以及**自动 K 推导**（Kneedle 肘部检测）和**断点继续（Checkpoint）** 功能。

[项目规范](./docs/requirements.md) | [整体架构](./docs/design.md)

---

## 在 VS2019 中运行

### 第 1 步：打开项目

用 VS2019 打开项目根目录，CMake 会自动识别（VS2019 自带 CMake 支持）。

> 如果 VS2019 没有自动检测，菜单栏选择：**文件 → 打开 → CMake...**，然后选择 `CMakeLists.txt`。

### 第 2 步：修改运行参数（可选）

打开 `src/main.cpp`，找到 `kmeans_run()` 函数，开头的变量就是可调参数：

```c++
// --- 数据文件路径（相对项目根目录） ---
std::string data_path = "data/a.dat";

// --- 计算后端 ---
// 可选值: "seq"  (串行)
//         "omp"  (OpenMP 多核)
//         "cuda" (GPU 加速)
std::string backend = "seq";

// --- 簇数 K ---
// true  = 自动推导最优 K（Kneedle 肘部检测）
// false = 使用下方 fixed_k 指定的值
bool auto_k = false;

// 手动指定 K（仅在 auto_k = false 时生效）
u32 fixed_k = 5;

// --- 迭代控制 ---
i32 max_iter    = 30;     // 最大迭代次数
f64 threshold   = 1e-4;   // 收敛阈值

// --- 读取方式 ---
bool streaming  = false;  // true = 流式读取（内存不足时使用）

// --- 断点继续 ---
std::string checkpoint_path = "";  // 非空则启用 checkpoint 功能
bool resume   = false;             // true = 从 checkpoint 恢复
```

改完直接保存即可。

### 第 3 步：运行

在 VS2019 中直接按 **F5**（调试运行）或 **Ctrl+F5**（直接运行），无需任何命令行参数。

运行结束后：
- 结果会打印在控制台
- 同时自动保存到 **`result_log/result_YYYYMMDD_HHMMSS.txt`** 文件

> 注意：项目根目录下的 `data/` 文件夹存放数据文件，数据格式为二进制 `(x, y)` 双精度浮点对，每点 16 字节。

---

## 在 Linux 终端中运行

### 环境要求

| 依赖 | 版本要求 |
|:-----|:---------|
| C++ 标准 | C++17 |
| CMake | $\ge$ 3.16 |
| OpenMP | $\ge$ 4.5 |
| CUDA (可选) | 支持 nvcc 编译 |
| MPI (可选) | OpenMPI $\ge$ 4.0 / MPICH $\ge$ 3.3 |

### 构建

```bash
# 在项目根目录下
mkdir -p build && cd build

# Release 模式（默认，推荐）
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Debug 模式
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

构建完成后，可执行文件位于 `build/kmeans`。

### 运行（Linux 终端）

直接运行（无参数，使用 `kmeans_run()` 中的默认设置）：

```bash
./build/kmeans
```

如果要修改参数，编辑 `src/main.cpp` 中 `kmeans_run()` 函数开头的变量，然后重新编译即可。

---

## 数据文件格式

数据文件为纯二进制格式，存储一系列二维坐标点 $(x, y)$，每个点占 **16 字节**（小端序）：

```
偏移量 0:  x (f64, 8 bytes)
偏移量 8:  y (f64, 8 bytes)
偏移量 16: x (f64, 8 bytes)
偏移量 24: y (f64, 8 bytes)
...
```

项目 `data/` 目录下提供了测试数据：
- `a.dat` — 小数据集（约 27 MB，约 170 万个点）
- `b.dat` — 大数据集（约 970 MB，约 6000 万个点）

---

## 运行结果说明

每次运行后，程序自动将结果保存到 `result_log/` 目录下，文件名包含时间戳，例如 `result_20260607_093841.txt`。

结果文件内容示例：

```
========================================
  K-Means 聚类运行结果
  运行时间: 20260607_093841
========================================

--- 配置参数 ---
  数据文件:        data/a.dat
  计算后端:        Sequential
  自动推导 K:      否
  指定 K:          5
  最大迭代次数:    30
  收敛阈值:        1.0e-04
  ...

--- 运行结果 ---
  簇数 (K):          5
  迭代次数:          30
  SSE (Inertia):     8.971627e+12
  加载时间:          0.000 s
  迭代时间:          14.912 s
  总运行时间:        15.003 s
========================================
```

---

## 参数说明

| 变量名 | 说明 | 默认值 |
|:-------|:-----|:-------|
| `data_path` | 数据文件路径（相对项目根目录） | `data/a.dat` |
| `backend` | 计算后端：`seq` / `omp` / `cuda` | `seq` |
| `auto_k` | 是否自动推导最优 $K$ | `false` |
| `fixed_k` | 手动指定 $K$（`auto_k=false` 时生效） | `5` |
| `max_iter` | 最大迭代次数 | `30` |
| `threshold` | 收敛阈值 | `1e-4` |
| `streaming` | 是否使用流式读取 | `false` |
| `checkpoint_path` | checkpoint 文件路径（空=不启用） | `""` |
| `resume` | 是否从 checkpoint 恢复 | `false` |

---

## 项目结构

```
mpi-compute/
├── CMakeLists.txt              # 根构建文件（C++17 + CUDA + OpenMP）
├── README.md                   # 本文件 — 使用说明
├── include/                    # 头文件
│   ├── types.hpp               # 类型定义
│   ├── dataset.hpp             # 数据集管理
│   ├── kmeans.hpp              # K-Means 主接口
│   ├── kmeans_kernels.cuh      # CUDA kernel 声明
│   └── checkpoint.hpp          # 断点继续
├── src/                        # 源文件
│   ├── main.cpp                # 【入口】函数式调用，直接修改参数即可运行
│   ├── dataset.cpp             # 数据集实现
│   ├── kmeans.cpp              # K-Means 主类 + 自动 K 推导
│   ├── kmeans_sequential.cpp   # CPU 串行后端
│   ├── kmeans_omp.cpp          # OpenMP 并行后端
│   ├── kmeans_kernels.cu       # CUDA 后端
│   └── checkpoint.cpp          # 断点继续实现
├── data/                       # 输入/输出数据
├── script/                     # 工具脚本
│   ├── CMakeLists.txt
│   └── dat_out.cpp             # 数据预览工具
├── docs/                       # 文档
│   ├── design.md               # 设计文档
│   └── requirements.md         # 代码规范
├── build/                      # 构建产物（不提交到 git）
└── result_log/                 # 运行结果自动保存到此目录
```

---

## 文档

- [代码规范](./docs/requirements.md) — 编码风格、编译器参数、版本要求
- [设计文档](./docs/design.md) — 算法原理、并行架构、优化策略

---

## 许可证

本项目为教学项目。
