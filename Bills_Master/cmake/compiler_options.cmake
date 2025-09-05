# 设置 C++ 标准为 C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 为 Release 构建类型设置优化编译选项 ---
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -flto -march=native")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto -s -march=native")

# --- 定义 PCH (预编译头) 编译选项 ---
set(PCH_HEADER "${SOURCE_ROOT}/pch.hpp")

# 为 MSVC 定义一个包含文件路径的单参数
set(PCH_FLAG_MSVC "/FI${PCH_HEADER}")

# 为 GCC/Clang 定义一个包含两个参数的 CMake 列表 (用分号分隔)
set(PCH_FLAG_GCC_CLANG "-include;${PCH_HEADER}")

# 使用生成器表达式，根据编译器选择正确的变量
set(FORCE_INCLUDE_PCH "$<IF:$<CXX_COMPILER_ID:MSVC>,${PCH_FLAG_MSVC},${PCH_FLAG_GCC_CLANG}>")

# 定义通用的编译选项
set(COMMON_COMPILE_OPTIONS
    -Wall
    -fdiagnostics-color=always
    ${FORCE_INCLUDE_PCH} # 应用自动包含 PCH 的选项
)