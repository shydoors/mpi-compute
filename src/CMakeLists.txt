# ============================================================
# src/CMakeLists.txt — 主项目源文件
# ============================================================

# CUDA 源文件
set(CUDA_SOURCES
  kmeans_kernels.cu
)

# C++ 源文件
set(CXX_SOURCES
  main.cpp
  dataset.cpp
  kmeans.cpp
  kmeans_sequential.cpp
  kmeans_omp.cpp
  checkpoint.cpp
)

# 收集所有源文件
set(ALL_SOURCES
  ${CUDA_SOURCES}
  ${CXX_SOURCES}
)

# ============================================================
# 主可执行文件
# ============================================================
add_executable(kmeans ${ALL_SOURCES})

target_include_directories(kmeans PRIVATE
  ${CMAKE_PROJECT_DIR}
  ${CMAKE_PROJECT_DIR}/include
)

# ============================================================
# 链接库
# ============================================================
target_link_libraries(kmeans PRIVATE
  OpenMP::OpenMP_CXX
)

if(MPI_CXX_FOUND)
  target_include_directories(kmeans PRIVATE ${MPI_CXX_INCLUDE_DIRS})
  target_link_libraries(kmeans PRIVATE ${MPI_CXX_LIBRARIES})
  target_compile_definitions(kmeans PRIVATE USE_MPI)
endif()

# ============================================================
# CUDA 架构 — 自动检测或使用默认
# ============================================================
if(CMAKE_CUDA_COMPILER)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.23")
    set_property(TARGET kmeans PROPERTY CUDA_ARCHITECTURES "native")
  else()
    set_property(TARGET kmeans PROPERTY CUDA_ARCHITECTURES "70;75;80;86;89;90")
  endif()
endif()
