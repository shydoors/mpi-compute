# 本项目为教学项目

本项目实现了若干实例，4个小实验，k-means算法

[项目规范](./docs/requirements.md)

[整体架构](./docs/design.md)

[工程优化](./docs/design.md)

采用 自动推导 来获取 分类的簇

# 项目结构

```
mpi-compute/
├── CMakeLists.txt            # 根构建文件
├── data/                     # 输入/输出数据
│   ├── a.dat
│   ├── b.dat
│   ├── c.dat
│   └── d.dat
├── docs/
│   ├── design.md
│   ├── parallelism.md
│   └── requirements.md
├── include/                  # 头文件
├── src/                      # 源文件（主项目代码）
├── script/                   # 工具脚本
│   ├── CMakeLists.txt
│   └── dat_out.cpp
├── build/                    # 构建产物（不提交）
└── README.md
```