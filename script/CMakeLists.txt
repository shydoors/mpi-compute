# ============================================================
# script 子目录 — 工具脚本 / 预处理程序
# 二进制文件输出于 ${CMAKE_PROJECT_DIR}/script/build
# ============================================================

set(SCRIPT_BINARY_DIR "${CMAKE_PROJECT_DIR}/script/build")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SCRIPT_BINARY_DIR}")

# ------------------------------------------------
# 公共 include 路径（供所有 script 目标复用）
# ------------------------------------------------
set(SCRIPT_INCLUDE_DIRS
        ${CMAKE_PROJECT_DIR}
        ${CMAKE_PROJECT_DIR}/include
)

# ------------------------------------------------
# dat_out — 数据格式转换 / 预览工具
# ------------------------------------------------
add_executable(dat_out dat_out.cpp)
target_include_directories(dat_out PRIVATE ${SCRIPT_INCLUDE_DIRS})
target_link_libraries(dat_out PRIVATE OpenMP::OpenMP_CXX)

# ============================================================
# 未来可在此处添加更多脚本目标，例如：
#   add_executable(testrc testrc.cpp)
#   target_include_directories(testrc PRIVATE ${SCRIPT_INCLUDE_DIRS})
# ============================================================