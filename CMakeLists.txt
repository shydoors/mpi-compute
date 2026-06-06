# 规定-std=c++17 ,o2优化
# 设置编译器选项
# 配置项目的全局变量$Project_Dir为当前CMakeLists.txt所在的目录
# 然后把以上变量传递给子目录
set(CMAKE_PROJECT_DIR ${Project_Dir})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")


# 注意
# 分为两个编译模式
# 1. Debug模式
# 2. Release模式
# 这两种模式的区别在于编译器会使用不同的优化选项和调试信息选项来编译代码。

# Debug模式通常会包含调试信息，但是不会进行优化，以便于开发人员调试代码。-O0, -g -Wall -Wextra -Werror
# Release模式则会进行优化，但不会包含调试信息，以提高程序的性能。 -O2