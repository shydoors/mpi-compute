cmake_minimum_required(VERSION 3.16)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# ============================================================
# 启用 CUDA 语言支持
# ============================================================
project(mpi-compute
  VERSION 1.0.0
  DESCRIPTION "教学项目 — K-Means 聚类算法（CUDA + OpenMP + MPI）"
  LANGUAGES C CXX CUDA
)

# ============================================================
# C++ 标准要求
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CUDA 标准（与 C++17 对齐）
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
# ============================================================
# 项目根目录变量（供子目录使用）
# ============================================================
set(Project_Dir "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_PROJECT_DIR "${Project_Dir}")

# ============================================================
# 构建类型选择（默认 Release）
# ============================================================
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ============================================================
# 编译器选项
# ============================================================
# 通用编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Debug 模式
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
# Release 模式
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# CUDA 通用标志
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Wall -Wno-pedantic")
set(CMAKE_CUDA_FLAGS_DEBUG "-g -G -O0")
set(CMAKE_CUDA_FLAGS_RELEASE "-O3 -DNDEBUG --use_fast_math")

# ============================================================
# OpenMP 支持
# ============================================================
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  message(STATUS "OpenMP found: ${OpenMP_CXX_FLAGS}")
else()
  message(FATAL_ERROR "OpenMP is required but not found.")
endif()

# ============================================================
# MPI 支持（可选）
# ============================================================
find_package(MPI QUIET)
if(MPI_CXX_FOUND)
  add_definitions(-DUSE_MPI)
  message(STATUS "MPI found: ${MPI_CXX_LIBRARIES}")
else()
  message(STATUS "MPI not found — building without MPI support")
endif()

# ============================================================
# 头文件目录
# ============================================================
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
file(MAKE_DIRECTORY "${INCLUDE_DIR}")

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${INCLUDE_DIR}
)

if(MPI_CXX_FOUND)
  include_directories(${MPI_INCLUDE_DIRS})
endif()

# ============================================================
# 构建输出目录
# ============================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

# ============================================================
# 子目录
# ============================================================
add_subdirectory(src)
add_subdirectory(script)
add_subdirectory(code-stu)
add_subdirectory(code-tec)

