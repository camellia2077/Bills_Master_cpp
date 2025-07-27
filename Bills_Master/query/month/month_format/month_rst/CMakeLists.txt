# CMakeLists.txt for the reStructuredText Formatter Plugin

# 设置CMake最低版本要求
cmake_minimum_required(VERSION 3.10)

# 定义项目名称
project(MonthRstFormatterPlugin CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 定义插件目标 ---
# 使用 add_library 创建一个名为 "rst_formatter" 的动态库 (SHARED)
add_library(rst_formatter SHARED
    MonthRstFormat.cpp
    "${CMAKE_SOURCE_DIR}/query/month/common/ReportSorter.cpp"
)

# --- 配置包含目录 ---
# 使得像 #include "query/month/common/ReportData.h" 这样的包含能够被正确解析。
target_include_directories(rst_formatter PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

# --- 为 Release 构建类型添加编译和链接优化选项 ---
target_compile_options(rst_formatter PRIVATE
    $<$<CONFIG:Release>:-Os -flto -fvisibility=hidden -ffunction-sections -fdata-sections>
)

target_link_options(rst_formatter PRIVATE
    $<$<CONFIG:Release>:-Wl,--gc-sections -s>
)

# ======================================================================
# ==                      核心修改点 START                            ==
# ==          移除手动设置的输出目录，并规范化插件名称              ==
# ======================================================================

# --- 设置插件属性 ---
set_target_properties(rst_formatter PROPERTIES
    # 为了在不同平台上生成一致的文件名 (例如 rst_formatter.dll)，
    # 而不是 librst_formatter.so (Linux) 或 librst_formatter.dll (Windows 默认)
    # 我们将库的前缀 "lib" 去掉。
    PREFIX ""
)

# 移除了 LIBRARY_OUTPUT_DIRECTORY 设置。
# 现在，此插件的输出位置将由根 CMakeLists.txt 文件中定义的
# 全局变量 CMAKE_LIBRARY_OUTPUT_DIRECTORY 控制，
# 确保所有 DLL 和 EXE 都被统一输出到 build/bin 目录。

# ======================================================================
# ==                       核心修改点 END                             ==
# ======================================================================

# 打印状态信息
message(STATUS "Configured reStructuredText formatter plugin 'rst_formatter'.")