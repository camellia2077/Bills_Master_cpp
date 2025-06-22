# CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称
project(BillManager LANGUAGES CXX)

# 设置C++标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 自动查找所有源文件 ---
file(GLOB MAIN_SRC "main.cpp") 
file(GLOB PARSING_SOURCES "parsing/*.cpp") 
file(GLOB DATABASE_SOURCES "database/*.cpp") 
file(GLOB REPORTING_SOURCES "reporting/*.cpp") 

# --- 添加可执行文件目标 ---
add_executable(bill_manager
    ${MAIN_SRC}
    ${PARSING_SOURCES}
    ${DATABASE_SOURCES}
    ${REPORTING_SOURCES}
) 

# --- 添加所有模块目录到头文件搜索路径 ---
target_include_directories(bill_manager PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/database"
    "${CMAKE_CURRENT_SOURCE_DIR}/parsing"
    "${CMAKE_CURRENT_SOURCE_DIR}/reporting"
) 

# --- 添加高级编译和链接优化选项 ---
# 这些选项仅在 "Release" 构建模式下生效，例如通过
# cmake -DCMAKE_BUILD_TYPE=Release ..
# ------------------------------------

# 添加编译器优化选项
target_compile_options(bill_manager PRIVATE
    # -O3: 开启高级别优化
    # -march=native: 为本机CPU架构优化
    # -flto: 开启链接时优化 (编译阶段)
    $<$<CONFIG:Release>:-O3 -march=native -flto>
)

# 添加链接器优化选项
target_link_options(bill_manager PRIVATE
    # -flto: 开启链接时优化 (链接阶段)
    # -s: 剥离符号表，减小最终可执行文件体积
    $<$<CONFIG:Release>:-flto -s>
)

# --- 寻找并链接依赖库 (SQLite3) ---
# 这是更稳定、更直接的替代方案，不依赖 pkg-config 

# 1. 寻找 sqlite3.h 头文件
find_path(SQLITE3_INCLUDE_DIR sqlite3.h
    # 提示CMake在MinGW默认的include路径下寻找
    HINTS ${MINGW_PREFIX}/include 
    # 如果提示无效，直接指定一个绝对路径作为备用
    PATHS "C:/msys64/mingw64/include" 
)

# 2. 寻找 libsqlite3.a 库文件
find_library(SQLITE3_LIBRARY NAMES sqlite3
    # 提示CMake在MinGW默认的lib路径下寻找
    HINTS ${MINGW_PREFIX}/lib 
    # 如果提示无效，直接指定一个绝对路径作为备用
    PATHS "C:/msys64/mingw64/lib" 
)

# 3. 检查是否都已找到，然后链接它们
if(SQLITE3_INCLUDE_DIR AND SQLITE3_LIBRARY)
    message(STATUS "Found SQLite3 header: ${SQLITE3_INCLUDE_DIR}")
    message(STATUS "Found SQLite3 library: ${SQLITE3_LIBRARY}")
    # 将找到的头文件目录添加到项目中
    target_include_directories(bill_manager PUBLIC ${SQLITE3_INCLUDE_DIR}) 
    # 将找到的库文件链接到项目中
    target_link_libraries(bill_manager PRIVATE ${SQLITE3_LIBRARY}) 
else()
    # 如果找不到，则构建失败并给出明确提示
    message(FATAL_ERROR "Could not find SQLite3 library or headers. Please ensure 'mingw-w64-x86_64-sqlite3' is installed via pacman.")
endif()

# 打印状态信息，方便调试
message(STATUS "Project build configured successfully.") 