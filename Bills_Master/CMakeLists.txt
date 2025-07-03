# 设置 CMake 所需的最低版本
cmake_minimum_required(VERSION 3.10)

# 定义项目名称和所用语言
project(BillReprocessor VERSION "0.2.0" LANGUAGES CXX)

# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 为 Release 构建类型设置优化编译选项 ---
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -flto")

# --- 为 Release 构建类型设置优化链接选项 ---
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto -s")


# --- 查找外部依赖 ---
# nlohmann_json for JSON processing in the reprocessing module
find_package(nlohmann_json REQUIRED)
# NEW: SQLite3 for the database insertion module
find_package(SQLite3 REQUIRED)


# --- 定义可执行文件目标 ---
# 明确列出所有源文件，这是更可靠的做法
add_executable(
    bill_matser_app          # 可执行文件名
    main.cpp
    reprocessing/Reprocessor.cpp
    reprocessing/BillValidator.cpp
    reprocessing/BillModifier.cpp
    db_insert/DataProcessor.cpp
    db_insert/insert.cpp
    db_insert/parser.cpp
    query/QueryDb.cpp
    query/MonthlyQuery.cpp
    query/YearlyQuery.cpp
    
) 


# --- 为目标添加头文件搜索路径 ---
# UPDATED: Add the db_insert directory
target_include_directories(bill_matser_app PRIVATE
    "reprocessing"
    "db_insert"
    "query"
)


# --- 为目标链接库 ---
# UPDATED: Add the SQLite3 library
target_link_libraries(bill_matser_app PRIVATE
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
)


# --- 为目标设置通用编译选项 ---
target_compile_options(bill_matser_app PRIVATE -Wall)