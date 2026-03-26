# 设置 CMake 所需的最低版本 (必须在顶层)
cmake_minimum_required(VERSION 3.28)

# 可选编译器选择（在 project() 之前生效）
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/modules/compiler_select.cmake")

# 定义项目名称和所用语言 (必须在顶层)
project(BillReprocessor VERSION "0.2.8" LANGUAGES C CXX)

option(BILLS_CORE_BUILD_SHARED "Build bills_core as a shared library for CLI app" OFF)
if(WIN32 AND BILLS_CORE_BUILD_SHARED)
    message(FATAL_ERROR
        "Windows dist no longer supports BILLS_CORE_BUILD_SHARED=ON. "
        "bills_core must be linked statically into the CLI executable."
    )
endif()
option(BILLS_ENABLE_LTO_RELEASE
    "Enable LTO for Release builds"
    ON
)
option(BILLS_ENABLE_TIDY
    "Enable clang-tidy target configuration"
    OFF
)
option(BILLS_CLI_ENABLE_MODULES
    "Enable C++ modules pilot for bills_cli presentation layer"
    ON
)

# 将模块路径添加到CMake的搜索路径中
list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/modules"
)

# 引入项目设置模块 (现在只包含路径和输出目录设置)
include(cmake/project_setup.cmake)

# 引入编译器选项和PCH设置
include(cmake/compiler_options.cmake)

# 引入并查找外部依赖
include(cmake/dependencies.cmake)

# 引入核心库
add_subdirectory(
    "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/core"
    "${CMAKE_CURRENT_BINARY_DIR}/libs/core"
)

# 引入 IO 适配层独立库
add_subdirectory(
    "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/io"
    "${CMAKE_CURRENT_BINARY_DIR}/libs/io"
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
