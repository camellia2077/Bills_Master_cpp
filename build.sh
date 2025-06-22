#!/bin/bash

# 当任何命令失败时，立即退出脚本
set -e

echo "======================================================"
echo "  Bill Manager Project - CMake Build Script for Bash"
echo "======================================================"
echo

# 切换到脚本所在的目录
cd "$(dirname "$0")"
echo "Script location: $(pwd)"
echo

# --- 新增的逻辑：清理旧的构建目录 ---
# 检查 "build" 目录是否存在
if [ -d "build" ]; then
    echo "'build' directory found, removing it for a clean build..."
    # -r 表示递归删除, -f 表示强制删除不提示
    rm -rf build
fi

# 创建一个新的、干净的构建目录
echo "Creating a new 'build' directory..."
mkdir build
# -----------------------------------

# 进入构建目录
cd build

# 步骤 1: 配置 CMake 项目
echo "----- [STEP 1/2] Configuring project with CMake... -----"
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
echo

# 步骤 2: 编译项目
echo "----- [STEP 2/2] Building the project... -----"
cmake --build .
echo

echo "======================================================"
echo "  BUILD SUCCESSFUL!"
echo "======================================================"
echo "The release executable 'bill_manager.exe' is in: $(pwd)"
echo