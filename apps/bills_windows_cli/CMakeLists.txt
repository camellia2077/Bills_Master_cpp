# 设置 CMake 所需的最低版本 (必须在顶层)
cmake_minimum_required(VERSION 3.10)

# 可选编译器选择（在 project() 之前生效）
include(cmake/compiler_select.cmake)

# 定义项目名称和所用语言 (必须在顶层)
project(BillReprocessor VERSION "0.2.8" LANGUAGES C CXX)

option(BILLS_CORE_BUILD_SHARED "Build bills_core as a shared library for CLI app" OFF)
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

# Export format switches (Phase 3 / Step 3.1): default to MD-only build.
option(ENABLE_FMT_MD "Enable Markdown formatter plugins" ON)
option(ENABLE_FMT_RST "Enable reStructuredText formatter plugins" OFF)
option(ENABLE_FMT_TEX "Enable LaTeX formatter plugins" OFF)
option(ENABLE_FMT_TYP "Enable Typst formatter plugins" OFF)

# 将模块路径添加到CMake的搜索路径中
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 引入项目设置模块 (现在只包含路径和输出目录设置)
include(cmake/project_setup.cmake)

# 引入编译器选项和PCH设置
include(cmake/compiler_options.cmake)

# 引入并查找外部依赖
include(cmake/dependencies.cmake)

# 收集所有源文件到变量中
include(cmake/source_files.cmake)

# 定义可执行文件目标
include(cmake/targets.cmake)

# 定义插件构建逻辑并加载所有插件
include(cmake/plugins.cmake)

# --- 代码格式化工具 ---
include(AddFormatTarget)
include(AddTidyTarget)
