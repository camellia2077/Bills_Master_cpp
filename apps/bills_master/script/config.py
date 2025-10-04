#!/usr/bin/env python3

# ------------------------------------------------------------------------------
# 构建生成器 (Build Generator)
# ------------------------------------------------------------------------------
# 指定 CMake 使用的构建系统。
#
# 可用选项:
#   - "Ninja":          一个专注于速度的小型构建系统。
#                       推荐在所有平台上使用以获得更快的构建速度。
#                       确保你已经安装了 ninja (例如, `sudo apt-get install ninja-build`)。
#
#   - "Unix Makefiles": 使用传统的 `make`。
#                       如果你没有安装 Ninja，这是一个可靠的选择。
#                       在 Linux 和 macOS 上可用。
#
# 默认值: "Ninja"
# ------------------------------------------------------------------------------
BUILD_GENERATOR = "Ninja"

# 如果你想使用 Make，请注释掉上面的 "Ninja" 并取消下面一行的注释
# BUILD_GENERATOR = "Unix Makefiles"

# ------------------------------------------------------------------------------
# CMake 源码目录 (CMake Source Directory)
# ------------------------------------------------------------------------------
# 指定包含顶层 CMakeLists.txt 文件的目录的绝对路径。
# 注意：路径应该是包含 CMakeLists.txt 的文件夹，而不是文件本身。
#
# 示例 (Windows): r"C:\path\to\your\project"
# 示例 (Linux/macOS): "/path/to/your/project"
# ------------------------------------------------------------------------------
CMAKELIST_SOURCE_DIR = r"C:\Computer\my_github\github_cpp\bills_master\Bills_Master_cpp\apps\bills_master"