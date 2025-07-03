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
# SQLite3 for the database insertion module
find_package(SQLite3 REQUIRED) 


# --- MODIFIED: Define shared library for common logic ---
# 1. List all source files that are shared between executables.
set(SHARED_SOURCES
    reprocessing/Reprocessor.cpp
    reprocessing/BillValidator.cpp
    reprocessing/BillModifier.cpp
    db_insert/DataProcessor.cpp
    db_insert/insert.cpp
    db_insert/parser.cpp
    query/QueryDb.cpp
    query/MonthlyQuery.cpp
    query/YearlyQuery.cpp
    FileHandler/FileHandler.cpp
)

# 2. Create a static library from the shared source files.
add_library(bill_master_lib STATIC ${SHARED_SOURCES})

# 3. Apply include directories and linked libraries to the shared library.
#    Executables that link to this library will inherit these properties.
target_include_directories(bill_master_lib PUBLIC
    "reprocessing"
    "db_insert"
    "query"
    "FileHandler"
    "common"
)

target_link_libraries(bill_master_lib PUBLIC
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
)

# Set common compile options for the library code.
target_compile_options(bill_master_lib PRIVATE -Wall)


# --- MODIFIED: Define two executable targets ---

# Target 1: The interactive menu-based application
add_executable(bill_matser_app main.cpp)

# Target 2: The command-line interface application
add_executable(bill_master_cli main_commond.cpp)


# --- MODIFIED: Link each executable to the shared library ---
target_link_libraries(bill_matser_app PRIVATE bill_master_lib)
target_link_libraries(bill_master_cli PRIVATE bill_master_lib)