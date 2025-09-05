# 设置 CMake 所需的最低版本 (必须在顶层)
cmake_minimum_required(VERSION 3.10)

# 定义项目名称和所用语言 (必须在顶层)
project(BillReprocessor VERSION "0.2.1" LANGUAGES CXX)

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