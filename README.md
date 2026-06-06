# 本项目为教学项目

本项目实现了若干实例，4个小实验，k-means算法

[项目规范](./docs/requirements.md)

[整体架构](./docs/design.md)

[工程优化](./docs/design.md)

采用 自动推导 来获取 分类的簇

# 项目结构

```
[shydoors@myant mpi-compute]$ tree
.
├── CMakeLists.txt
├── data
│   ├── a.dat
│   └── b.dat
├── docs
│   ├── design.md
│   ├── parallelism.md
│   └── requirements.md
├── include
├── README.md
├── script
│   ├── check_env.cpp
│   ├── CMakeLists.txt
│   └── dat_out.cpp
├── src
├── tmp
├── tmp.1.md
└── tmp.md
```