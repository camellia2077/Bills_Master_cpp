# 设置 CMake 所需的最低版本
cmake_minimum_required(VERSION 3.10)

# 定义项目名称和所用语言
project(BillReprocessor VERSION "0.2.1" LANGUAGES CXX)

# 设置 C++ 标准为 C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 为 Release 构建类型设置优化编译选项 ---
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -flto -march=native")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto -s -march=native")

# --- 统一定义输出目录 ---
set(OUTPUT_BINARY_DIR "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_BINARY_DIR})

# --- 查找外部依赖 ---
find_package(nlohmann_json REQUIRED)
set(SQLite3_USE_STATIC_LIBS OFF)
find_package(SQLite3 REQUIRED)



# --- 宏观路径定义 ---
set(REPROCESSING_DIR "reprocessing")
set(DB_INSERT_DIR    "db_insert")
set(QUERY_DIR        "query")
set(FILEHANDLER_DIR  "file_handler")
set(APPCONTROLLER_DIR "app_controller")
# 新增: 为 command_line 帮助文档定义路径
set(USAGE_HELP_DIR   "usage_help")


# ==============================================================================
#  源文件收集
# ==============================================================================

# 1. reprocessing 模块源文件
file(GLOB REPROCESSING_SOURCES
    "${REPROCESSING_DIR}/*.cpp"
    "${REPROCESSING_DIR}/validator/*.cpp"
    "${REPROCESSING_DIR}/validator/config/*.cpp"
    "${REPROCESSING_DIR}/validator/result/*.cpp"
    "${REPROCESSING_DIR}/validator/verifier/*.cpp"
    "${REPROCESSING_DIR}/modifier/*.cpp"
    "${REPROCESSING_DIR}/modifier/config_loader/*.cpp"
    "${REPROCESSING_DIR}/modifier/processor/*.cpp" 
    "${REPROCESSING_DIR}/modifier/raw_format/*.cpp" 
)


file(GLOB DB_INSERT_SOURCES
    "${DB_INSERT_DIR}/*.cpp" 
    "${DB_INSERT_DIR}/bill_structures/*.cpp" 
    "${DB_INSERT_DIR}/insertor/*.cpp" 
    "${DB_INSERT_DIR}/parser/*.cpp" 
)
file(GLOB QUERY_SOURCES
    "${QUERY_DIR}/*.cpp" 
    
    "${QUERY_DIR}/components/*.cpp" 
    "${QUERY_DIR}/components/common/*.cpp" 
    "${QUERY_DIR}/components/monthly_report/*.cpp" 
    "${QUERY_DIR}/components/yearly_report/*.cpp" 

    "${QUERY_DIR}/core/*.cpp" 

    "${QUERY_DIR}/plugins/*.cpp" 
    "${QUERY_DIR}/plugins/month_formatters/*.cpp" 
    # 移除插件源文件
    # 以下各行注释掉，因为这些格式化器现在将作为独立的动态库被编译。
    # 将它们包含在这里会导致“重复定义”的链接错误。
    #"${QUERY_DIR}/plugins/month_formatters/month_md/*.cpp" 
    #"${QUERY_DIR}/plugins/month_formatters/month_rst/*.cpp" 
    #"${QUERY_DIR}/plugins/month_formatters/month_tex/*.cpp" 
    #"${QUERY_DIR}/plugins/month_formatters/month_typ/*.cpp" 

    "${QUERY_DIR}/plugins/year_formatters/*.cpp" 
    # 移除插件源文件
    # 以下各行注释掉，因为这些格式化器现在将作为独立的动态库被编译。
    # 将它们包含在这里会导致“重复定义”的链接错误。
    #"${QUERY_DIR}/plugins/year_formatters/year_md/*.cpp" 
    #"${QUERY_DIR}/plugins/year_formatters/year_rst/*.cpp" 
    #"${QUERY_DIR}/plugins/year_formatters/year_tex/*.cpp" 
    #"${QUERY_DIR}/plugins/year_formatters/year_typ/*.cpp" 
)
file(GLOB FILEHANDLER_SOURCES "${FILEHANDLER_DIR}/*.cpp")
file(GLOB APPCONTROLLER_SOURCES "${APPCONTROLLER_DIR}/*.cpp")

# 新增: 收集 usage_help 目录下的源文件
file(GLOB USAGE_HELP_SOURCES "${USAGE_HELP_DIR}/*.cpp")

# 将所有找到的、用于主程序的源文件合并到一个列表中
set(SHARED_SOURCES
    ${REPROCESSING_SOURCES}
    ${DB_INSERT_SOURCES}
    ${QUERY_SOURCES}
    ${FILEHANDLER_SOURCES}
    ${APPCONTROLLER_SOURCES}
)

# ==============================================================================
# ==       核心修改点 2: 确保 wrapper.cpp 不被包含          ==
# ==============================================================================
# 确保 wrapper.cpp 不会被意外地编译进您的 C++ 主程序。
list(REMOVE_ITEM SHARED_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${REPROCESSING_DIR}/wrapper.cpp")


# ==============================================================================
#  可执行文件目标定义
# ==============================================================================
# GUI/TUI 版本，使用共享源文件
add_executable(bill_matser_app main.cpp ${SHARED_SOURCES})

# CLI 版本，使用共享源文件和独立的帮助文档源文件
add_executable(bill_master_cli main_command.cpp ${SHARED_SOURCES} ${USAGE_HELP_SOURCES})

foreach(target bill_matser_app bill_master_cli)
    # 添加包含目录
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    # 以启用预编译头文件
    target_precompile_headers(${target} PRIVATE
        "${CMAKE_SOURCE_DIR}/common/pch.h"
    )
    # 链接外部库
    target_link_libraries(${target} PRIVATE
        nlohmann_json::nlohmann_json
        SQLite::SQLite3
        stdc++exp
    )

    # 设置编译选项
    target_compile_options(${target} PRIVATE -Wall -fdiagnostics-color=always)
endforeach()


# ==============================================================================
#  插件构建函数 (在此处添加新函数)
# ==============================================================================
# 定义一个函数，用于简化所有格式化器插件的创建过程
# 参数1 (name): 插件的目标名称, 例如 "md_month_formatter"
# 参数2+ (sources): 该插件需要的所有源文件
function(create_formatter_plugin name)
    # 将函数的所有额外参数 (源文件) 收集到一个列表中
    set(sources ${ARGN})

    # 创建动态库 (SHARED)
    add_library(${name} SHARED ${sources})

    # 设置头文件包含目录 (所有插件都需要访问项目根目录的头文件)
    target_include_directories(${name} PRIVATE
        "${CMAKE_SOURCE_DIR}"
    )

    # 设置通用的编译优化选项 (适用于所有插件)
    target_compile_options(${name} PRIVATE
        $<$<CONFIG:Release>:-Os -flto -fvisibility=hidden -ffunction-sections -fdata-sections>
    )

    # 设置通用的链接优化选项 (适用于所有插件)
    target_link_options(${name} PRIVATE
        $<$<CONFIG:Release>:-Wl,--gc-sections -s>
    )

    # 设置通用的目标属性 (移除 "lib" 前缀，确保跨平台文件名一致)
    set_target_properties(${name} PROPERTIES
        PREFIX ""
    )

    # 打印状态信息，方便调试
    message(STATUS "Configured formatter plugin '${name}' via function.")
endfunction()

#  加载插件模块
message(STATUS "Loading report formatter plugins...")
# --- 月度报告插件 ---
add_subdirectory(query/plugins/month_formatters/month_md)
add_subdirectory(query/plugins/month_formatters/month_rst)
add_subdirectory(query/plugins/month_formatters/month_tex)
add_subdirectory(query/plugins/month_formatters/month_typ)


# 添加年度报告插件的子目录
# --- 年度报告插件 ---
add_subdirectory(query/plugins/year_formatters/year_md/)
add_subdirectory(query/plugins/year_formatters/year_tex)
add_subdirectory(query/plugins/year_formatters/year_rst)
add_subdirectory(query/plugins/year_formatters/year_typ)

message(STATUS "All plugins configured.")


# 移除整个 Python 模块编译部分(因为这个是主程序不是封装python模块的程序)
# 以下所有与 pybind11_add_module 和 reprocessing_engine 相关的代码块已被完全删除。 


# --- 自动复制第三方 DLL (无变化) ---
if(WIN32 AND SQLite3_LIBRARIES)
    foreach(target bill_matser_app bill_master_cli)
        add_custom_command(
            TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SQLite3_LIBRARIES}"
            $<TARGET_FILE_DIR:${target}>
            COMMENT "Copying SQLite3 DLL to output directory for ${target}" 
        )
    endforeach()
endif()