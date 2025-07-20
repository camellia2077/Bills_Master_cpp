# CMakeLists.txt

# 要求最低的 CMake 版本
cmake_minimum_required(VERSION 3.12)
# 项目名称
project(ReprocessorPybind)

# --- 查找依赖 ---

# 查找 Python 解释器和库
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# 查找在 UCRT64 环境中已安装的 nlohmann_json 库
find_package(nlohmann_json CONFIG REQUIRED)

# 将本地的 pybind11 目录作为子项目包含进来
add_subdirectory(pybind11)


# --- 手动指定所有源文件 ---
# 不再使用 file(GLOB_RECURSE)，而是明确列出每个编译单元。
# 这提供了更精确的控制，并且是大型项目中推荐的做法。
set(SOURCES
    reprocessing/wrapper.cpp
    reprocessing/Reprocessor.cpp
    reprocessing/modifier/BillModifier.cpp
    reprocessing/modifier/config_loader/ConfigLoader.cpp
    reprocessing/modifier/processor/BillProcessor.cpp
    reprocessing/modifier/raw_format/BillFormatter.cpp
    reprocessing/validator/BillConfig.cpp
    reprocessing/validator/BillProcessor.cpp
    reprocessing/validator/BillValidator.cpp
    reprocessing/validator/ValidationResult.cpp
)


# --- 创建 Python 模块 ---
pybind11_add_module(reprocessor_lib ${SOURCES})


# --- 指定头文件目录 ---
# 此命令告诉编译器在编译 reprocessor_lib 时，
# 将 "reprocessing" 目录作为一个顶层搜索路径。
# 这样，像 #include "validator/BillValidator.h" 这样的语句就能被正确解析。
target_include_directories(reprocessor_lib PRIVATE reprocessing)


# --- 链接依赖库 ---
# 链接顺序和内容保持不变
target_link_libraries(reprocessor_lib PRIVATE 
    Python3::Python
    nlohmann_json::nlohmann_json
)
