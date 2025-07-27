# CMakeLists.txt for the Markdown Formatter Plugin

# 设置CMake最低版本要求
cmake_minimum_required(VERSION 3.10)

# 定义项目名称
project(MonthMdFormatterPlugin CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 定义插件目标 ---
# 使用 add_library 创建一个名为 "md_formatter" 的动态库 (SHARED)
add_library(md_formatter SHARED
    MonthMdFormat.cpp
    # CORRECTED LINE: Use CMAKE_SOURCE_DIR for a reliable absolute path
    "${CMAKE_SOURCE_DIR}/query/month/common/ReportSorter.cpp"
)

# --- 配置包含目录 ---
target_include_directories(md_formatter PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

# ======================================================================
# ==                      核心修改点 START                            ==
# ==            为 Release 构建类型添加编译和链接优化选项             ==
# ======================================================================

# --- 编译选项 (Compiler Flags) ---
target_compile_options(md_formatter PRIVATE
    # $<BUILD_TYPE:Release> 是一个生成器表达式，表示这些选项仅在 Release 模式下生效
    # -Os: 优化大小。相比 -O3 (优化速度)，编译器会更倾向于生成更小的代码。
    # -flto: 启用链接时优化(Link-Time Optimization)。允许编译器在链接阶段跨文件进行优化，能有效减小体积。
    # -fvisibility=hidden: 将所有符号的可见性默认设为“隐藏”。这是优化共享库体积最关键的一步。
    # -ffunction-sections 和 -fdata-sections: 告诉编译器将每个函数和数据片段放入独立的节区。
    $<$<CONFIG:Release>:-Os -flto -fvisibility=hidden -ffunction-sections -fdata-sections>
)

# --- 链接选项 (Linker Flags) ---
target_link_options(md_formatter PRIVATE
    # --gc-sections: (垃圾回收) 链接器会移除所有未被引用的代码节区，需要与上面的-ffunction-sections配合。
    # -s: 剥离 (Strip) 所有符号表和重定位信息，能显著减小最终文件的大小。
    $<$<CONFIG:Release>:-Wl,--gc-sections -s>
)

# ======================================================================
# ==                       核心修改点 END                             ==
# ======================================================================

# --- 设置输出目录 ---
# 定义插件生成的位置。我们将其统一放在项目根目录下的 "plugins" 文件夹中。
set_target_properties(md_formatter PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/plugins"
    # 为了在不同平台上生成一致的文件名 (例如 md_formatter.dll)，而不是 libmd_formatter.so
    PREFIX ""
)

# 打印状态信息
message(STATUS "Configured Markdown formatter plugin 'md_formatter'.")