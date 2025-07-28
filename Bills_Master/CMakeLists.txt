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



# --- 宏观路径定义 (无变化) ---
set(REPROCESSING_DIR "reprocessing")
set(DB_INSERT_DIR    "db_insert")
set(QUERY_DIR        "query")
set(FILEHANDLER_DIR  "file_handler")
set(APPCONTROLLER_DIR "app_controller")


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
    "${QUERY_DIR}/metadata_reader/*.cpp" 
    "${QUERY_DIR}/export_bills/*.cpp" 
    "${QUERY_DIR}/month/*.cpp" 
    "${QUERY_DIR}/month/_month_data/*.cpp" 
    "${QUERY_DIR}/month/common/*.cpp" 
    "${QUERY_DIR}/month/month_format/*.cpp" 
    # 移除插件源文件
    # 以下各行注释掉，因为这些格式化器现在将作为独立的动态库被编译。
    # 将它们包含在这里会导致“重复定义”的链接错误。
    # "${QUERY_DIR}/month/month_format/month_md/*.cpp"
    # "${QUERY_DIR}/month/month_format/month_tex/*.cpp"
    # "${QUERY_DIR}/month/month_format/month_typ/*.cpp"
    # "${QUERY_DIR}/month/month_format/month_rst/*.cpp"
    "${QUERY_DIR}/month/month_query/*.cpp" 
    "${QUERY_DIR}/year/*.cpp" 
    "${QUERY_DIR}/year/_year_data/*.cpp" 
    "${QUERY_DIR}/year/year_format/*.cpp" 
    # ==========================================================
    # ==      核心修改点 1: 移除年度报告插件的源文件        ==
    # ==========================================================
    # "${QUERY_DIR}/year/year_format/year_md/*.cpp" 
    # "${QUERY_DIR}/year/year_format/year_tex/*.cpp" 
    # "${QUERY_DIR}/year/year_format/year_typ/*.cpp" 
    # "${QUERY_DIR}/year/year_format/year_rst/*.cpp" 
    "${QUERY_DIR}/year/year_query/*.cpp" 
)
file(GLOB FILEHANDLER_SOURCES "${FILEHANDLER_DIR}/*.cpp")
file(GLOB APPCONTROLLER_SOURCES "${APPCONTROLLER_DIR}/*.cpp")

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
#  可执行文件目标定义 (无变化)
# ==============================================================================
add_executable(bill_matser_app main.cpp ${SHARED_SOURCES})
add_executable(bill_master_cli main_command.cpp ${SHARED_SOURCES})

foreach(target bill_matser_app bill_master_cli)
    # 添加包含目录
    # 我们只需要提供一个路径：项目的根目录。
    # CMAKE_CURRENT_SOURCE_DIR 在根 CMakeLists.txt 中就代表项目根目录。
    # PRIVATE 表示这个设置只对当前目标生效，不会传递给依赖它的其他目标。
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    # 链接外部库 (无变化)
    target_link_libraries(${target} PRIVATE
        nlohmann_json::nlohmann_json
        SQLite::SQLite3
        stdc++exp
    )

    # 设置编译选项 (无变化)
    target_compile_options(${target} PRIVATE -Wall -fdiagnostics-color=always)
endforeach()


# ==============================================================================
#  加载插件模块
# ==============================================================================
message(STATUS "Loading report formatter plugins...")
# --- 月度报告插件 ---
add_subdirectory(query/month/month_format/month_md)
add_subdirectory(query/month/month_format/month_rst)
add_subdirectory(query/month/month_format/month_tex)
add_subdirectory(query/month/month_format/month_typ)

# ==========================================================
# ==      核心修改点 2: 添加年度报告插件的子目录        ==
# ==========================================================
# --- 年度报告插件 ---
add_subdirectory(query/year/year_format/year_md)
add_subdirectory(query/year/year_format/year_tex)
add_subdirectory(query/year/year_format/year_rst)
add_subdirectory(query/year/year_format/year_typ)

message(STATUS "All plugins configured.")

# ==============================================================================
# ==           核心修改点 3: 移除整个 Python 模块编译部分                 ==
# ==============================================================================
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