# 设置 C++ 标准为 C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# [New] 生成编译数据库 (compile_commands.json)，供 clang-tidy/tools 使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include("${CMAKE_CURRENT_LIST_DIR}/../../../cmake/modules/windows_static_runtime.cmake")

if(WIN32 AND MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Use lld for MinGW+Clang to avoid noisy GNU ld duplicate section warnings.
    foreach(_bills_flag_var
        CMAKE_C_FLAGS
        CMAKE_CXX_FLAGS
        CMAKE_EXE_LINKER_FLAGS
        CMAKE_SHARED_LINKER_FLAGS
        CMAKE_MODULE_LINKER_FLAGS
    )
        if(NOT "${${_bills_flag_var}}" MATCHES "(^| )-fuse-ld=lld($| )")
            set(${_bills_flag_var} "${${_bills_flag_var}} -fuse-ld=lld")
        endif()
    endforeach()
endif()

# --- 为 Release 构建类型设置优化编译选项 ---
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os -march=native")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -march=native")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s -march=native")

set(_BILLS_CAN_USE_LTO TRUE)
if(WIN32 AND MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Static release link currently fails under MinGW Clang+lto/lld on this project.
    set(_BILLS_CAN_USE_LTO FALSE)
    message(WARNING
        "BILLS_ENABLE_LTO_RELEASE requested, but disabled for MinGW Clang "
        "because static release linking is not stable with LTO on this project."
    )
endif()

if(BILLS_ENABLE_LTO_RELEASE AND NOT MSVC AND _BILLS_CAN_USE_LTO)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")

    if(WIN32 AND MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # MinGW + Clang LTO must use lld to avoid GNU ld LLVMgold plugin failures.
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fuse-ld=lld")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fuse-ld=lld")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE
            "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -fuse-ld=lld")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE
            "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -fuse-ld=lld")
        set(CMAKE_MODULE_LINKER_FLAGS_RELEASE
            "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} -fuse-ld=lld")
    endif()
endif()

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
)

set(COMMON_LINK_OPTIONS)
if(NOT MSVC)
    list(APPEND COMMON_COMPILE_OPTIONS
        -ffunction-sections
        -fdata-sections
    )
    list(APPEND COMMON_LINK_OPTIONS
        -Wl,--gc-sections
    )
endif()
