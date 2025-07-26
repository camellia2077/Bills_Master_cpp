# 设置 CMake 所需的最低版本
cmake_minimum_required(VERSION 3.10)

# 定义项目名称和所用语言
project(BillReprocessor VERSION "0.2.1" LANGUAGES CXX)

# 设置 C++ 标准为 C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 为 Release 构建类型设置优化编译选项 ---
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
    "${QUERY_DIR}/month/month_format/*.cpp"
    "${QUERY_DIR}/month/month_format/month_md/*.cpp"
    "${QUERY_DIR}/month/month_format/month_tex/*.cpp"
    "${QUERY_DIR}/month/month_format/month_typ/*.cpp"
    "${QUERY_DIR}/month/month_format/month_rst/*.cpp"
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
#  库和可执行文件目标定义 (Library and Executable Target Definition)
# ==============================================================================

# 1. 从共享源文件创建一个静态库
add_library(bill_master_lib STATIC ${SHARED_SOURCES})

# 2. 为共享库应用包含目录和链接库
#    链接到此库的可执行文件将继承这些属性。
# --- MODIFIED: 将项目根目录和所有核心模块的根目录都添加到包含路径中 ---
# 这允许在代码中使用更短、更相对的 #include 路径。
target_include_directories(bill_master_lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    # 添加核心模块的根目录
    "${CMAKE_CURRENT_SOURCE_DIR}/${REPROCESSING_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/${DB_INSERT_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/${QUERY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/${FILEHANDLER_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/${APPCONTROLLER_DIR}"
)

target_link_libraries(bill_master_lib PUBLIC
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
    stdc++exp
)

# 为库代码设置通用的编译选项
target_compile_options(bill_master_lib PRIVATE -Wall -fdiagnostics-color=always)

# --- 可执行文件目标 ---

# Target 1: The interactive menu-based application
add_executable(bill_matser_app main.cpp)

# Target 2: The command-line interface application
add_executable(bill_master_cli main_command.cpp)


# 将每个可执行文件链接到共享库
target_link_libraries(bill_matser_app PRIVATE bill_master_lib)
target_link_libraries(bill_master_cli PRIVATE bill_master_lib)