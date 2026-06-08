# 语义准确

1. 使用 `cstdint` 库来声明整数类型

* `std::int32_t` ==> `int`
* `std::uint32_t` ==> `unsigned int`
* `std::int64_t` ==> `long long`
* `std::uint64_t` ==> `unsigned long long`

2. 类的初始化使用大括号（统一列表初始化）。
3. 不得省略括号、大括号。尤其 for 循环、if 语句判断的时候。
4. 优先使用 `char*` 作为文件路径，其次使用 `std::filesystem::path`，再其次 `std::string`。
5. 读写文件时，不得使用文件的绝对地址，只准用相对地址。项目目录作为根目录。
6. 禁止全局使用命名空间（`using namespace xxx;`），显式调用对应命名空间的函数。
7. 优先使用 `const` 和 `constexpr`，避免使用 `#define` 定义常量。
8. 智能指针优先于裸指针，避免手动 `new/delete`。
9. 函数参数传递：非基本类型使用 `const &`（只读）或 `&`（可写），避免不必要的拷贝。

# 版本

| KEY | VALUE |
|:----|:------|
| C++ 标准 | C++17 |
| OpenMP | >= 4.5 |
| CMake | >= 3.16 |
| nvcc 主机侧（gcc/g++） | gcc (GCC) 16.1.1 20260430 |
| nvcc 主机侧（msvc） | VS2019 |

# 编译器参数

| KEY | VALUE |
|:----|:------|
| DEBUG | `-g -O0 -std=c++17 -Wall -Wextra -fopenmp` |
| RELEASE | `-O2 -std=c++17 -Wall -Wextra -fopenmp` |

# 项目结构

* `src/` — 源文件
* `include/` — 头文件
* `build/` — 构建产物（不提交到 git）
* `code-stu/` — 学习代码、课本示例（自动扫描编译）
* `data/` — 输入数据
* `results/` — 输出文件，以时间戳为文件夹（不提交到 git）
* `docs/` — 文档
* `script/` — 脚本，包括但不限于预处理文件
