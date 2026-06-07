# mpi-compute — K-Means 聚类算法（教学项目）

本项目实现了 **K-Means 聚类算法**，支持 **CUDA / OpenMP / 串行** 多后端并行加速，以及**自动 K 推导**（Kneedle 肘部检测）和**断点继续（Checkpoint）** 功能。

[项目规范](./docs/requirements.md) | [整体架构](./docs/design.md)

---

## 在 VS2019 中运行

### 第 1 步：打开项目

用 VS2019 打开项目根目录，CMake 会自动识别。

> 菜单：**文件 -> 打开 -> CMake...**，选择 `CMakeLists.txt` 。

### 第 2 步：修改配置（可选）

打开 `src/config.json` ，修改运行参数：

```json
{
    "data_path": "data/a.dat",
    "backend": "omp",
    "auto_k": false,
    "fixed_k": 5,
    "max_iterations": 30,
    "threshold": 1e-4,
    "streaming": false,
    "checkpoint_path": "",
    "resume": false,
    "output": {
        "render_max_points": 100000
    }
}
```

改完保存即可，**无需重新编译**。

### 第 3 步：运行

直接按 **F5** 或 **Ctrl+F5**。

运行结束后，所有输出文件自动保存到 `results/` 目录。

---

## 在 Linux 终端中运行

### 环境要求

| 依赖 | 版本要求 |
|:-----|:---------|
| C++ 标准 | C++17 |
| CMake | $\ge$ 3.16 |
| OpenMP | $\ge$ 4.5 |
| CUDA (可选) | 支持 nvcc 编译 |

### 构建与运行

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..
./build/kmeans
```

修改参数请编辑 `src/config.json` ，无需重新编译。

---

## 配置文件说明

| 字段 | 说明 | 默认值 |
|:-----|:-----|:-------|
| `data_path` | 数据文件路径（相对项目根目录） | `data/a.dat` |
| `backend` | 计算后端： `seq` / `omp` / `cuda` | `omp` |
| `auto_k` | 是否自动推导最优 $K$ | `false` |
| `fixed_k` | 手动指定 $K$（ `auto_k=false` 时生效） | `5` |
| `max_iterations` | 最大迭代次数 | `30` |
| `threshold` | 收敛阈值 | `1e-4` |
| `streaming` | 是否使用流式读取 | `false` |
| `checkpoint_path` | checkpoint 文件路径（空=不启用） | `""` |
| `resume` | 是否从 checkpoint 恢复 | `false` |
| `output.render_max_points` | 渲染 CSV 最多采样点数 | `100000` |

---

## 输出文件说明

每次运行后， `results/` 目录下生成以下文件：

### `result_YYYYMMDD_HHMMSS.txt` — 运行日志

配置参数 + 运行结果（K 值、迭代次数、SSE、耗时），纯文本。

### `labels_YYYYMMDD_HHMMSS.bin` — 全部聚类标签（二进制，不丢失数据）

**最重要的输出文件**，保存了所有点的聚类结果。

**格式：**

| 偏移 | 类型 | 内容 |
|:-----|:-----|:-----|
| 0 | `u64` | 总点数 $N$ |
| 8 | `u32[N]` | 每个点所属的簇编号（$0 \sim K-1$） |

**Python 读取：**

```python
import numpy as np
with open("labels_20260607_100351.bin", "rb") as f:
    n = np.frombuffer(f.read(8), dtype=np.uint64)[0]
    labels = np.frombuffer(f.read(), dtype=np.uint32)
```

### `centers_YYYYMMDD_HHMMSS.csv` — 各簇中心坐标

```
cx,cy,cluster
3644.28,9778.86,0
9814.15,2732.08,1
...
```

### `render_YYYYMMDD_HHMMSS.csv` — 渲染用采样点

当数据量超过 `render_max_points` （默认 10 万）时自动均匀采样，避免渲染卡死。

```python
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("render_20260607_100351.csv")
plt.scatter(df["x"], df["y"], c=df["label"], s=1, cmap="tab10")
plt.savefig("clusters.png", dpi=300)
```

---

## 数据文件格式

纯二进制， `double (x, y)` 坐标对，每点 16 字节（小端序）：

```
偏移 0:  x (f64, 8 bytes)
偏移 8:  y (f64, 8 bytes)
...
```

`data/` 目录下的测试数据：
* `a.dat` — ~27 MB，约 170 万个点
* `b.dat` — ~970 MB，约 6000 万个点

---

## 项目结构

```
mpi-compute/
├── CMakeLists.txt              # 构建文件
├── README.md                   # 本文件
├── .gitignore                  # 确定哪些文件不被记录追踪
├── src/
│   ├── config.json             # 运行配置（改此文件，无需重新编译）
│   ├── main.cpp                # 入口（读 config.json 运行）
│   ├── kmeans.cpp
│   ├── kmeans_sequential.cpp
│   ├── kmeans_omp.cpp
│   ├── kmeans_kernels.cu
│   ├── dataset.cpp
│   └── checkpoint.cpp
├── include/
│   ├── types.hpp
│   ├── dataset.hpp
│   ├── kmeans.hpp
│   ├── kmeans_kernels.cuh
│   └── checkpoint.hpp
├── data/                       # 输入数据（不提交）
│   └── .gitkeep
├── docs/
│   ├── design.md               # 设计文档
│   └── requirements.md         # 代码规范
├── build/                      # 构建产物（不提交）
├── results/                    # 运行结果（不提交）
│   └── .gitkeep
└── script/
    ├── CMakeLists.txt
    └── dat_out.cpp
```

---

## 文档

* [代码规范](./docs/requirements.md)
* [设计文档](./docs/design.md)
