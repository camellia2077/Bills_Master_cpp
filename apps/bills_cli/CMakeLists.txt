# 设置 CMake 所需的最低版本 (必须在顶层)
cmake_minimum_required(VERSION 3.10)

# 可选编译器选择（在 project() 之前生效）
include(cmake/compiler_select.cmake)

# 定义项目名称和所用语言 (必须在顶层)
project(BillReprocessor VERSION "0.2.8" LANGUAGES C CXX)

option(BILLS_CORE_BUILD_SHARED "Build bills_core as a shared library for CLI app" ON)
if(WIN32)
    set(_BILLS_STATIC_MINGW_RUNTIME_DEFAULT ON)
else()
    set(_BILLS_STATIC_MINGW_RUNTIME_DEFAULT OFF)
endif()
option(BILLS_STATIC_MINGW_RUNTIME
    "Prefer static libgcc/libstdc++/winpthread on Windows MinGW builds"
    ${_BILLS_STATIC_MINGW_RUNTIME_DEFAULT}
)
unset(_BILLS_STATIC_MINGW_RUNTIME_DEFAULT)
option(BILLS_ENABLE_LTO_RELEASE
    "Enable LTO for Release builds"
    ON
)
option(BILLS_ENABLE_TIDY
    "Enable clang-tidy target configuration"
    OFF
)

# Export format switches: built-in report formatter selection.
option(ENABLE_FMT_MD "Enable built-in Markdown report formatters" ON)
option(ENABLE_FMT_RST "Enable built-in reStructuredText report formatters" OFF)
option(ENABLE_FMT_TEX "Enable built-in LaTeX report formatters" OFF)

set(BILLS_FORMAT_COMPILE_DEFINITIONS)
if(ENABLE_FMT_MD)
    list(APPEND BILLS_FORMAT_COMPILE_DEFINITIONS BILLS_FMT_MD_ENABLED=1)
else()
    list(APPEND BILLS_FORMAT_COMPILE_DEFINITIONS BILLS_FMT_MD_ENABLED=0)
endif()
if(ENABLE_FMT_RST)
    list(APPEND BILLS_FORMAT_COMPILE_DEFINITIONS BILLS_FMT_RST_ENABLED=1)
else()
    list(APPEND BILLS_FORMAT_COMPILE_DEFINITIONS BILLS_FMT_RST_ENABLED=0)
endif()
if(ENABLE_FMT_TEX)
    list(APPEND BILLS_FORMAT_COMPILE_DEFINITIONS BILLS_FMT_TEX_ENABLED=1)
else()
    list(APPEND BILLS_FORMAT_COMPILE_DEFINITIONS BILLS_FMT_TEX_ENABLED=0)
endif()

# 将模块路径添加到CMake的搜索路径中
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 引入项目设置模块 (现在只包含路径和输出目录设置)
include(cmake/project_setup.cmake)

# 引入编译器选项和PCH设置
include(cmake/compiler_options.cmake)

# 引入并查找外部依赖
include(cmake/dependencies.cmake)

# 引入 IO 适配层独立库
add_subdirectory(
    "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/bills_io"
    "${CMAKE_CURRENT_BINARY_DIR}/libs/bills_io"
)

# 收集所有源文件到变量中
include(cmake/source_files.cmake)

# 定义可执行文件目标
include(cmake/targets.cmake)

# 定义插件构建逻辑并加载所有插件
include(cmake/report_formatters.cmake)

# --- 代码格式化工具 ---
include(AddFormatTarget)
if(BILLS_ENABLE_TIDY)
    include(AddTidyTarget)
else()
    message(STATUS "BILLS_ENABLE_TIDY=OFF, skip configuring tidy target.")
endif()
