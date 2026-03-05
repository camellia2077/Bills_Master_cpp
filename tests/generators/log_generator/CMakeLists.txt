# CMake最低版本要求
cmake_minimum_required(VERSION 3.12)

# 可选编译器选择（在 project() 之前生效）
include(cmake/compiler_select.cmake)

# 定义项目名称和语言
project(BillGenerator CXX)

# 将模块路径添加到CMake的搜索路径中
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 引入项目设置与编译选项
include(cmake/project_setup.cmake)
include(cmake/compiler_options.cmake)

# 引入并查找外部依赖
include(cmake/dependencies.cmake)

# 收集源文件与目标定义
include(cmake/source_files.cmake)
include(cmake/targets.cmake)

# --- 代码格式化与静态分析工具 ---
include(AddFormatTarget)
include(AddTidyFixTarget)

# --- 用户反馈信息 ---
message(STATUS "Project configured successfully. Executable will be named 'generator'.")
message(STATUS "To build, run: cmake --build .")
message(STATUS "The output will be in the 'build/bin' directory.")
