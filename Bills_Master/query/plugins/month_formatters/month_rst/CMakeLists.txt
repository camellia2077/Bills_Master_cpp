# CMakeLists.txt for the reStructuredText Formatter Plugin

# 设置CMake最低版本要求
cmake_minimum_required(VERSION 3.10)

# 定义项目名称
project(MonthRstFormatterPlugin CXX)


# 将dll插件的目标名称存储在一个变量中
set(PLUGIN_TARGET_NAME "rst_month_formatter")

# 设置C++标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 定义插件目标 ---
# 使用 add_library 创建一个名为 "rst_month_formatter" 的动态库 (SHARED)
add_library(${PLUGIN_TARGET_NAME} SHARED
    MonthRstFormat.cpp
    "${CMAKE_SOURCE_DIR}/query/components/monthly_report/ReportSorter.cpp" 
)

# --- 配置包含目录 ---
# 使得像 #include "query/month/common/ReportData.h" 这样的包含能够被正确解析。
target_include_directories(${PLUGIN_TARGET_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

# --- 为 Release 构建类型添加编译和链接优化选项 ---
target_compile_options(${PLUGIN_TARGET_NAME} PRIVATE
    $<$<CONFIG:Release>:-Os -flto -fvisibility=hidden -ffunction-sections -fdata-sections>
)

target_link_options(${PLUGIN_TARGET_NAME} PRIVATE
    $<$<CONFIG:Release>:-Wl,--gc-sections -s>
)

# --- 设置插件属性 ---
set_target_properties(${PLUGIN_TARGET_NAME} PROPERTIES
    # 为了在不同平台上生成一致的文件名 (例如 rst_month_formatter.dll)，
    # 而不是 librst_month_formatter.so (Linux) 或 librst_month_formatter.dll (Windows 默认)
    # 我们将库的前缀 "lib" 去掉。
    PREFIX ""
)

# 移除了 LIBRARY_OUTPUT_DIRECTORY 设置。
# 现在，此插件的输出位置将由根 CMakeLists.txt 文件中定义的
# 全局变量 CMAKE_LIBRARY_OUTPUT_DIRECTORY 控制，
# 确保所有 DLL 和 EXE 都被统一输出到 build/bin 目录。

# 打印状态信息
message(STATUS "Configured reStructuredText formatter plugin 'rst_month_formatter'.")