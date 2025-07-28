# CMakeLists.txt for the Typst Formatter Plugin

# 设置CMake最低版本要求
cmake_minimum_required(VERSION 3.10)

# 定义项目名称
project(MonthTypFormatterPlugin CXX)


# 将dll插件的目标名称存储在一个变量中
set(PLUGIN_TARGET_NAME "typ_month_formatter")

# 设置C++标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 定义插件目标 ---
# 使用 add_library 创建一个名为 "typ_month_formatter" 的动态库 (SHARED)
add_library(${PLUGIN_TARGET_NAME} SHARED
    MonthTypFormat.cpp
    # 使用 CMAKE_SOURCE_DIR 保证无论在何处构建，都能找到公共代码的绝对路径
    "${CMAKE_SOURCE_DIR}/query/month/common/ReportSorter.cpp"
)

# --- 配置包含目录 ---
# 告诉编译器去项目的根目录查找头文件。
target_include_directories(${PLUGIN_TARGET_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

# --- 为 Release 构建类型添加编译和链接优化选项 ---
target_compile_options(${PLUGIN_TARGET_NAME} PRIVATE
    $<$<CONFIG:Release>:-Os -flto -fvisibility=hidden -ffunction-sections -fdata-sections>
)

target_link_options(typ_month_formatter PRIVATE
    $<$<CONFIG:Release>:-Wl,--gc-sections -s>
)

# --- 设置插件属性 ---
set_target_properties(${PLUGIN_TARGET_NAME} PROPERTIES
    # 为了在不同平台上生成一致的文件名 (例如 typ_month_formatter.dll)，
    # 而不是 libtyp_month_formatter.so (Linux) 或 libtyp_month_formatter.dll (Windows 默认)
    # 我们将库的前缀 "lib" 去掉。
    PREFIX ""
)

# 移除了 LIBRARY_OUTPUT_DIRECTORY 设置。
# 现在，此插件的输出位置将由根 CMakeLists.txt 文件中定义的
# 全局变量 CMAKE_LIBRARY_OUTPUT_DIRECTORY 控制，
# 确保所有 DLL 和 EXE 都被统一输出到 build/bin 目录。

# 打印状态信息，方便调试
message(STATUS "Configured Typst formatter plugin 'typ_month_formatter'.")