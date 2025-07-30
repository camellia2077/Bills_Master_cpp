# CMakeLists.txt for the Yearly Markdown Formatter Plugin

# 设置CMake最低版本要求
cmake_minimum_required(VERSION 3.10)

# 定义项目名称
project(YearMdFormatterPlugin CXX)

# 将dll插件的目标名称存储在一个变量中
set(PLUGIN_TARGET_NAME "md_year_formatter")

# 设置C++标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 定义插件目标 ---
# 使用变量 ${PLUGIN_TARGET_NAME} 来创建库
# 注意：这里我们假设接口和基类实现在项目的根目录中，
# 如果它们在其他地方，需要相应地调整路径。
add_library(${PLUGIN_TARGET_NAME} SHARED
    YearMdFormat.cpp
)

# --- 配置包含目录 ---
# 确保编译器能找到 IYearlyReportFormatter.h 和其他必要的头文件
target_include_directories(${PLUGIN_TARGET_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

# --- 为 Release 构建类型添加编译和链接优化选项 (可选，但推荐) ---
target_compile_options(${PLUGIN_TARGET_NAME} PRIVATE
    $<$<CONFIG:Release>:-Os -flto -fvisibility=hidden -ffunction-sections -fdata-sections>
)

target_link_options(${PLUGIN_TARGET_NAME} PRIVATE
    $<$<CONFIG:Release>:-Wl,--gc-sections -s>
)

# --- 设置插件属性 ---
set_target_properties(${PLUGIN_TARGET_NAME} PROPERTIES
    # 为了在不同平台上生成一致的文件名 (例如 md_year_formatter.dll)，
    # 我们将库的前缀 "lib" 去掉。
    PREFIX ""
)

# 打印状态信息
message(STATUS "Configured Yearly Markdown formatter plugin '${PLUGIN_TARGET_NAME}'.")