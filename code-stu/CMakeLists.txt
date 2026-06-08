# ============================================================
# code-stu 子目录 — 程序示例
# 自动扫描所有 .c/.cpp/.cc/.cu 文件并编译为同名的可执行文件
# 二进制文件输出于 ${CMAKE_PROJECT_DIR}/code-stu/build
# ============================================================

set(SCRIPT_BINARY_DIR "${CMAKE_PROJECT_DIR}/code-stu/build")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SCRIPT_BINARY_DIR}")

find_package(CUDA QUIET)
find_package(MPI QUIET)

file(GLOB SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cu"
)

foreach(SRC ${SRC_FILES})
    get_filename_component(FULL_NAME ${SRC} NAME)
    string(REGEX REPLACE "\\.[^.]*$" "" EXE_NAME "${FULL_NAME}")
    get_filename_component(EXT ${SRC} LAST_EXT)

    if(EXT STREQUAL ".cu")
        if(NOT CUDA_FOUND)
            message(STATUS "  [SKIP] ${EXE_NAME} (需要 CUDA)")
            continue()
        endif()
        cuda_add_executable(${EXE_NAME} ${SRC})
        target_compile_options(${EXE_NAME} PRIVATE -std=c++17)
        message(STATUS "  [ADD] ${EXE_NAME} <- ${FULL_NAME} (CUDA)")
    elseif(EXT STREQUAL ".c")
        add_executable(${EXE_NAME} ${SRC})
        set_target_properties(${EXE_NAME} PROPERTIES LINKER_LANGUAGE C)
        target_compile_features(${EXE_NAME} PRIVATE c_std_17)
        message(STATUS "  [ADD] ${EXE_NAME} <- ${FULL_NAME}")
    else()
        add_executable(${EXE_NAME} ${SRC})
        target_compile_features(${EXE_NAME} PRIVATE cxx_std_17)
        message(STATUS "  [ADD] ${EXE_NAME} <- ${FULL_NAME}")
    endif()

    target_include_directories(${EXE_NAME} PRIVATE
        ${CMAKE_PROJECT_DIR} ${CMAKE_PROJECT_DIR}/include)

    if(OpenMP_CXX_FOUND AND EXT STREQUAL ".cpp")
        target_link_libraries(${EXE_NAME} PRIVATE OpenMP::OpenMP_CXX)
    endif()
    if(MPI_FOUND AND (EXT STREQUAL ".cpp" OR EXT STREQUAL ".cc"))
        if(EXT STREQUAL ".c")
            target_link_libraries(${EXE_NAME} PRIVATE MPI::MPI_C)
        else()
            target_link_libraries(${EXE_NAME} PRIVATE MPI::MPI_CXX)
        endif()
    endif()

    set_target_properties(${EXE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${SCRIPT_BINARY_DIR}")
endforeach()
