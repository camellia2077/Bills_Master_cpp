# 设置 CMake 所需的最低版本
cmake_minimum_required(VERSION 3.10)

# 定义项目名称和所用语言
project(BillReprocessor VERSION "0.2.1" LANGUAGES CXX)

# 设置 C++ 标准为 C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 为 Release 构建类型设置优化编译选项 ---
# 注意 同时开启Os和 flto编译的时候，会因为json库导致编译失败 所以不能同时使用Os和flto
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -march=native") 
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -flto -march=native")

# --- 为 Release 构建类型设置优化链接选项 ---
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto -s -march=native")


# --- 查找外部依赖 ---
# nlohmann_json for JSON processing in the reprocessing module
find_package(nlohmann_json REQUIRED)
# SQLite3 for the database insertion module
set(SQLite3_USE_STATIC_LIBS OFF) # 添加此行以请求动态库 
find_package(SQLite3 REQUIRED)


# ==============================================================================
#  宏观路径定义 (Path Macros Definition)
# ==============================================================================
# 定义各个核心模块的根目录路径
set(REPROCESSING_DIR "reprocessing") 
set(DB_INSERT_DIR    "db_insert") 
set(QUERY_DIR        "query") 
set(FILEHANDLER_DIR  "file_handler") 
set(APPCONTROLLER_DIR "app_controller") 


# ==============================================================================
#  源文件收集 (Source File Collection)
# ==============================================================================
# 使用上面定义的宏观路径来自动查找每个模块下的源文件 (*.cpp)

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
# 去除wrapper.cpp，因为这是python模块封装的程序
list(REMOVE_ITEM REPROCESSING_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${REPROCESSING_DIR}/wrapper.cpp") 

# 2. db_insert 模块源文件
file(GLOB DB_INSERT_SOURCES
    "${DB_INSERT_DIR}/*.cpp" 
    "${DB_INSERT_DIR}/bill_structures/*.cpp" 
    "${DB_INSERT_DIR}/insertor/*.cpp" 
    "${DB_INSERT_DIR}/parser/*.cpp" 
)

# 3. query 模块源文件
file(GLOB QUERY_SOURCES
    "${QUERY_DIR}/*.cpp" 
    "${QUERY_DIR}/export_bills/*.cpp" 
    "${QUERY_DIR}/month/*.cpp" 
    "${QUERY_DIR}/month/_month_data/*.cpp" 
    "${QUERY_DIR}/month/common/*.cpp" 
    "${QUERY_DIR}/month/month_format/*.cpp" 
    # =================================================================================
    # ==                 核心修改点 1: 移除插件源文件                  ==
    # =================================================================================
    # 以下各行已被移除或注释掉，因为这些格式化器现在将作为独立的动态库被编译。
    # 将它们包含在这里会导致“重复定义”的链接错误。
    # "${QUERY_DIR}/month/month_format/month_md/*.cpp"
    # "${QUERY_DIR}/month/month_format/month_tex/*.cpp"
    # "${QUERY_DIR}/month/month_format/month_typ/*.cpp"
    # "${QUERY_DIR}/month/month_format/month_rst/*.cpp"
    # =================================================================================
    "${QUERY_DIR}/month/month_query/*.cpp" 
    "${QUERY_DIR}/year/*.cpp" 
    "${QUERY_DIR}/year/_year_data/*.cpp" 
    "${QUERY_DIR}/year/year_format/*.cpp" 
    "${QUERY_DIR}/year/year_format/year_md/*.cpp" 
    "${QUERY_DIR}/year/year_format/year_tex/*.cpp" 
    "${QUERY_DIR}/year/year_format/year_typ/*.cpp" 
    "${QUERY_DIR}/year/year_format/year_rst/*.cpp" 
    "${QUERY_DIR}/year/year_query/*.cpp" 
)

# 4. 其他模块源文件
file(GLOB FILEHANDLER_SOURCES "${FILEHANDLER_DIR}/*.cpp") 
file(GLOB APPCONTROLLER_SOURCES "${APPCONTROLLER_DIR}/*.cpp") 

# 将所有找到的源文件合并到一个列表中
set(SHARED_SOURCES
    ${REPROCESSING_SOURCES}
    ${DB_INSERT_SOURCES}
    ${QUERY_SOURCES}
    ${FILEHANDLER_SOURCES}
    ${APPCONTROLLER_SOURCES}
)


# ==============================================================================
#  可执行文件目标定义 (Executable Target Definition)
# ==============================================================================

# Target 1: The interactive menu-based application
add_executable(bill_matser_app main.cpp ${SHARED_SOURCES})

# Target 2: The command-line interface application
add_executable(bill_master_cli main_command.cpp ${SHARED_SOURCES})

# 为两个可执行文件设置通用的属性
foreach(target bill_matser_app bill_master_cli)
    # 添加包含目录
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/${REPROCESSING_DIR}" 
        "${CMAKE_CURRENT_SOURCE_DIR}/${DB_INSERT_DIR}" 
        "${CMAKE_CURRENT_SOURCE_DIR}/${QUERY_DIR}" 
        "${CMAKE_CURRENT_SOURCE_DIR}/${FILEHANDLER_DIR}" 
        "${CMAKE_CURRENT_SOURCE_DIR}/${APPCONTROLLER_DIR}" 
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
#  加载插件模块 (Load Plugin Modules)
# ==============================================================================
# =================================================================================
# ==                  核心修改点 2: 添加所有插件子目录                  ==
# =================================================================================
# 使用 add_subdirectory 指令来包含并处理每个插件的 CMakeLists.txt 文件。
# CMake会进入这些目录，执行其中的CMake指令，并创建我们定义的动态库目标。

message(STATUS "Loading report formatter plugins...")
add_subdirectory(query/month/month_format/month_md)
add_subdirectory(query/month/month_format/month_rst)
add_subdirectory(query/month/month_format/month_tex)
add_subdirectory(query/month/month_format/month_typ)
message(STATUS "All plugins configured.")