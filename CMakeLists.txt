# 规定-std=c++17 ,o2优化
# 设置编译器选项
# 配置项目的全局变量$Project_Dir为当前CMakeLists.txt所在的目录
set(Project_Dir ${CMAKE_CURRENT_SOURCE_DIR})
# 然后传递给子目录


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")   