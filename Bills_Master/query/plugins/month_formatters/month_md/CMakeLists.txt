# CMakeLists.txt for the Markdown Formatter Plugin

# 设置CMake最低版本要求
cmake_minimum_required(VERSION 3.10)

# 定义项目名称
project(MonthMdFormatterPlugin CXX)

# 将dll插件的目标名称存储在一个变量中
set(PLUGIN_TARGET_NAME "md_month_formatter")


# 设置C++标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 定义插件目标 ---
# 使用变量 ${PLUGIN_TARGET_NAME} 来创建库
add_library(${PLUGIN_TARGET_NAME} SHARED
    MonthMdFormat.cpp
    "${CMAKE_SOURCE_DIR}/query/components/monthly_report/ReportSorter.cpp" 
)

# --- 配置包含目录 ---
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
    # 为了在不同平台上生成一致的文件名 (例如 md_month_formatter.dll)，
    # 我们将库的前缀 "lib" 去掉。
    PREFIX ""
)

# 打印状态信息
message(STATUS "Configured Markdown formatter plugin '${PLUGIN_TARGET_NAME}'.")