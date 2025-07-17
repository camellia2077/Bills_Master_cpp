# 设置 CMake 所需的最低版本
cmake_minimum_required(VERSION 3.10)

# 定义项目名称和所用语言
project(BillReprocessor VERSION "0.2.1" LANGUAGES CXX)

# 设置 C++ 标准为 C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 为 Release 构建类型设置优化编译选项 ---
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -flto")

# --- 为 Release 构建类型设置优化链接选项 ---
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto -s")


# --- 查找外部依赖 ---
# nlohmann_json for JSON processing in the reprocessing module
find_package(nlohmann_json REQUIRED)
# SQLite3 for the database insertion module
find_package(SQLite3 REQUIRED)


# 1. 自动从每个子目录中查找源文件 (*.cpp)
file(GLOB REPROCESSING_SOURCES 
    "reprocessing/*.cpp"
    "reprocessing/validator/*.cpp"
    "reprocessing/modifier/*.cpp"
    "reprocessing/modifier/config_loader/*.cpp"
    "reprocessing/modifier/processor/*.cpp"
    "reprocessing/modifier/raw_format/*.cpp"
    
)
file(GLOB DB_INSERT_SOURCES   
    "db_insert/*.cpp"
    "db_insert/bill_structures/*.cpp"
    "db_insert/insertor/*.cpp"
    "db_insert/parser/*.cpp"
)
file(GLOB QUERY_SOURCES       
    "query/*.cpp"
    "query/export/*.cpp"


    "query/year/*.cpp"
    "query/year/_year_data/*.cpp"
    "query/year/year_format/*.cpp"
    "query/year/year_query/*.cpp"

    "query/month/*.cpp"
    "query/month/_month_data/*.cpp"
    "query/month/month_format/*.cpp"
    "query/month/month_query/*.cpp"

)
file(GLOB FILEHANDLER_SOURCES "file_handler/*.cpp")
file(GLOB APPCONTROLLER_SOURCES "app_controller/*.cpp")

# 将所有找到的源文件合并到一个列表中
set(SHARED_SOURCES
    ${REPROCESSING_SOURCES}
    ${DB_INSERT_SOURCES}
    ${QUERY_SOURCES}
    ${FILEHANDLER_SOURCES}
    ${APPCONTROLLER_SOURCES}
)

# 2. 从共享源文件创建一个静态库
add_library(bill_master_lib STATIC ${SHARED_SOURCES})

# 3. 为共享库应用包含目录和链接库
#    链接到此库的可执行文件将继承这些属性。
target_include_directories(bill_master_lib PUBLIC
    "reprocessing"
    "db_insert"
    "query"
    "file_handler"
    "common"
    "app_controller"
)

target_link_libraries(bill_master_lib PUBLIC
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
    stdc++exp
)

# 为库代码设置通用的编译选项
target_compile_options(bill_master_lib PRIVATE -Wall -fdiagnostics-color=always)

# Target 1: The interactive menu-based application
add_executable(bill_matser_app main.cpp)

# Target 2: The command-line interface application
add_executable(bill_master_cli main_command.cpp)


# --- MODIFIED: Link each executable to the shared library ---
target_link_libraries(bill_matser_app PRIVATE bill_master_lib)
target_link_libraries(bill_master_cli PRIVATE bill_master_lib)