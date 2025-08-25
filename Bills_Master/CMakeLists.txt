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
set(SOURCE_ROOT "src")

set(REPROCESSING_DIR "${SOURCE_ROOT}/reprocessing")
set(DB_INSERT_DIR    "${SOURCE_ROOT}/db_insert")
set(QUERY_DIR        "${SOURCE_ROOT}/query")
set(FILEHANDLER_DIR  "${SOURCE_ROOT}/file_handler")
set(APPCONTROLLER_DIR "${SOURCE_ROOT}/app_controller")
set(USAGE_HELP_DIR   "${SOURCE_ROOT}/usage_help")


# ==============================================================================
#  源文件收集 (已从 file(GLOB) 修改为显式列出)
# ==============================================================================

# 1. reprocessing 模块源文件
set(REPROCESSING_SOURCES
    "${REPROCESSING_DIR}/Reprocessor.cpp"
    "${REPROCESSING_DIR}/modifier/BillModifier.cpp"
    "${REPROCESSING_DIR}/modifier/config_loader/ConfigLoader.cpp"
    "${REPROCESSING_DIR}/modifier/processor/BillContentTransformer.cpp"
    "${REPROCESSING_DIR}/modifier/raw_format/BillFormatter.cpp"
    "${REPROCESSING_DIR}/validator/BillValidator.cpp"
    "${REPROCESSING_DIR}/validator/config/BillConfig.cpp"
    "${REPROCESSING_DIR}/validator/result/ValidationResult.cpp"
    "${REPROCESSING_DIR}/validator/verifier/BillFormatVerifier.cpp"
)

# 2. db_insert 模块源文件
set(DB_INSERT_SOURCES
    "${DB_INSERT_DIR}/DataProcessor.cpp"
    "${DB_INSERT_DIR}/insertor/BillInserter.cpp"
    "${DB_INSERT_DIR}/parser/BillParser.cpp"
)

# 3. query 模块源文件 (不包含插件, 插件由 add_subdirectory 处理)
set(QUERY_SOURCES
    "${QUERY_DIR}/components/monthly_report/MonthlyReportGenerator.cpp"
    "${QUERY_DIR}/components/monthly_report/MonthQuery.cpp"
    "${QUERY_DIR}/components/monthly_report/ReportSorter.cpp"
    "${QUERY_DIR}/components/yearly_report/YearlyReportGenerator.cpp"
    "${QUERY_DIR}/components/yearly_report/YearQuery.cpp"
    "${QUERY_DIR}/core/BillMetadataReader.cpp"
    "${QUERY_DIR}/core/QueryDb.cpp"
    "${QUERY_DIR}/core/ReportExporter.cpp"
    "${QUERY_DIR}/plugins/year_formatters/BaseYearlyReportFormatter.cpp"
)

# 4. file_handler 模块源文件
set(FILEHANDLER_SOURCES
    "${FILEHANDLER_DIR}/FileHandler.cpp"
)

# 5. app_controller 模块源文件
set(APPCONTROLLER_SOURCES
    "${APPCONTROLLER_DIR}/AppController.cpp"
    "${APPCONTROLLER_DIR}/ExportController.cpp"
    "${APPCONTROLLER_DIR}/WorkflowController.cpp"
)

# 6. usage_help 模块源文件
set(USAGE_HELP_SOURCES
    "${USAGE_HELP_DIR}/usage_help.cpp"
)

# 将所有用于主程序的源文件合并到一个列表中
set(SHARED_SOURCES
    ${REPROCESSING_SOURCES}
    ${DB_INSERT_SOURCES}
    ${QUERY_SOURCES}
    ${FILEHANDLER_SOURCES}
    ${APPCONTROLLER_SOURCES}
)

# ==============================================================================
# 确保 wrapper.cpp 不被包含
# ==============================================================================
# 由于我们是手动列出文件，只要不把 wrapper.cpp 添加到 REPROCESSING_SOURCES 列表中，
# 它就不会被编译，因此不再需要 list(REMOVE_ITEM)。


# ==============================================================================
#  可执行文件目标定义
# ==============================================================================
# GUI/TUI 版本
add_executable(bill_matser_app
    "${SOURCE_ROOT}/main.cpp"
    ${SHARED_SOURCES}
)

# CLI 版本
add_executable(bill_master_cli
    "${SOURCE_ROOT}/main_command.cpp"
    ${SHARED_SOURCES}
    ${USAGE_HELP_SOURCES}
)

foreach(target bill_matser_app bill_master_cli)
    # 添加包含目录
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_ROOT}"
    )
    # 以启用预编译头文件
    target_precompile_headers(${target} PRIVATE
        "${CMAKE_SOURCE_DIR}/${SOURCE_ROOT}/common/pch.h"
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
#  插件构建函数
# ==============================================================================
function(create_formatter_plugin name)
    set(sources ${ARGN})
    add_library(${name} SHARED ${sources})
    target_include_directories(${name} PRIVATE
        "${CMAKE_SOURCE_DIR}/${SOURCE_ROOT}"
    )
    target_compile_options(${name} PRIVATE
        $<$<CONFIG:Release>:-Os -flto -fvisibility=hidden -ffunction-sections -fdata-sections>
    )
    target_link_options(${name} PRIVATE
        $<$<CONFIG:Release>:-Wl,--gc-sections -s>
    )
    set_target_properties(${name} PROPERTIES
        PREFIX ""
    )
    message(STATUS "Configured formatter plugin '${name}' via function.")
endfunction()

# ==============================================================================
#  加载插件模块
# ==============================================================================
message(STATUS "Loading report formatter plugins...")
# --- 月度报告插件 ---
add_subdirectory(${SOURCE_ROOT}/query/plugins/month_formatters/month_md)
add_subdirectory(${SOURCE_ROOT}/query/plugins/month_formatters/month_rst)
add_subdirectory(${SOURCE_ROOT}/query/plugins/month_formatters/month_tex)
add_subdirectory(${SOURCE_ROOT}/query/plugins/month_formatters/month_typ)

# --- 年度报告插件 ---
add_subdirectory(${SOURCE_ROOT}/query/plugins/year_formatters/year_md/)
add_subdirectory(${SOURCE_ROOT}/query/plugins/year_formatters/year_tex)
add_subdirectory(${SOURCE_ROOT}/query/plugins/year_formatters/year_rst)
add_subdirectory(${SOURCE_ROOT}/query/plugins/year_formatters/year_typ)

message(STATUS "All plugins configured.")


# ==============================================================================
#  自动复制第三方 DLL
# ==============================================================================
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