#!/bin/bash

# 切换到脚本所在的目录
# 这确保了无论从哪里运行此脚本，它都能找到项目文件
SCRIPT_DIR=$(dirname "$0")
cd "$SCRIPT_DIR" || { echo "Error: Could not change to script directory."; exit 1; }

echo "Current directory: $(pwd)"

# 定义构建目录名称
BUILD_DIR="build"

# 检查并删除旧的构建目录
if [ -d "$BUILD_DIR" ]; then
    echo "Deleting existing build directory: $BUILD_DIR"
    rm -rf "$BUILD_DIR"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to delete build directory. Exiting."
        exit 1
    fi
else
    echo "No existing build directory found. Skipping deletion."
fi

# 创建新的构建目录
echo "Creating new build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"
if [ $? -ne 0 ]; then
    echo "Error: Failed to create build directory. Exiting."
    exit 1
fi

# 进入构建目录
cd "$BUILD_DIR" || { echo "Error: Could not change to build directory."; exit 1; }

# 运行 CMake 配置项目
# .. 表示 CMakeLists.txt 在上一级目录 (即项目根目录)
echo "Running CMake to configure project..."
# 显式指定生成器为 Ninja
cmake -G "Ninja" ..
if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed. Exiting."
    exit 1
fi

# 运行 Ninja 编译项目 (替换 make)
echo "Running Ninja to compile project..."
ninja -j$(nproc) # Ninja 默认也支持 -j 参数进行并行编译
if [ $? -ne 0 ]; then
    echo "Error: Compilation failed. Exiting."
    exit 1
fi

echo "Build process completed successfully!"
echo "Executable can be found in: $(pwd)/generator"

# 返回到脚本的原始目录（可选，但通常是个好习惯）
cd - > /dev/null
