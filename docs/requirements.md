# 语义准确

1. 使用 `cstdint`库来声明整数类型

* `std::int32_t` ==> `int`
* `std::int64_t` ==> `long long`
* `std::uint32_t` ==> `unsigned int`
* `std::uint64_t` ==> `unsigned long long`

2. 类的初始化使用大括号。
3. 使用`std::filesystem::path`代替`std::string`，来实现文件路径
4. 禁止全局使用命名空间，显式调用对应命名空间的函数。
5. 不得使用绝对地址，只准用相对地址，项目目录作为根目录。

# 

# 版本

| KEY          | VALUE                     |
|:-------------|:--------------------------|
| c++标准        | c++17                     |
| omp库         |                           |
| mpi库         |                           |
| CMAKE        | $ >=3.16 $                |
| nvcc-gcc/g++ | gcc (GCC) 16.1.1 20260430 |
| nvcc-msvc    | vs2019                    |

# 编译器参数

| KEY     | VALUE                               |
|:--------|:------------------------------------|
| DEBUG   | `-g -std=c++17 -std=c++17 -fopenmp` |
| RELEASE | `-std=c++17 -fopenmp`               |



# 语义准确

1. 使用 `cstdint`库来声明整数类型

* `std::int32_t` ==> `int`
* `std::uint32_t` ==> `unsigned int`
* `std::int64_t` ==> `long long`
* `std::uint64_t` ==> `unsigned long long`

2. 类的初始化使用大括号（统一列表初始化）。
3. 使用`std::filesystem::path`代替`std::string`，来实现文件路径。
4. 禁止全局使用命名空间（`using namespace xxx;`），显式调用对应命名空间的函数。
5. 不得使用绝对地址，只准用相对地址，项目目录作为根目录。
6. 优先使用 `const` 和 `constexpr`，避免使用 `#define` 定义常量。
7. 智能指针优先于裸指针，避免手动 `new/delete`。
8. 函数参数传递：非基本类型使用 `const &`（只读）或 `&`（可写），避免不必要的拷贝。

# 版本

| KEY          | VALUE                         |
|:-------------|:------------------------------|
| C++ 标准       | C++17                         |
| OpenMP       | >= 4.5                        |
| MPI          | OpenMPI >= 4.0 / MPICH >= 3.3 |
| CMake        | >= 3.16                       |
| nvcc-gcc/g++ | gcc (GCC) 16.1.1 20260430     |
| nvcc-msvc    | VS2019                        |

# 编译器参数

| KEY     | VALUE                                      |
|:--------|:-------------------------------------------|
| DEBUG   | `-g -O0 -std=c++17 -Wall -Wextra -fopenmp` |
| RELEASE | `-O2 -std=c++17 -Wall -Wextra -fopenmp`    |

# 项目结构

* `src/` — 源文件
* `include/` — 头文件
* `build/` — 构建产物（不提交到 git）
* `data/` — 输入/输出数据
* `docs/` — 文档