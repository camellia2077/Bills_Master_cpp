# CMake最低版本要求
cmake_minimum_required(VERSION 3.12)

# 定义项目名称和语言
project(BillGenerator CXX) 

# --- C++标准与编译器设置 ---
set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- 查找依赖项 ---
find_package(nlohmann_json 3.2.0 CONFIG REQUIRED) 

# --- 添加可执行文件目标 ---
# *** 修改: 更新所有源文件的路径以包含 src/ 前缀 ***
add_executable(generator
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/_internal/BillGenerator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/_internal/arg_parser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/_internal/utils.cpp"
)

# --- 设置可执行文件的输出目录 ---
# *** 新增: 将编译出的 exe 文件放入 build/bin 目录 ***
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# --- 添加头文件目录 ---
# *** 修改: 更新头文件搜索路径 ***
target_include_directories(generator PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/_internal")

# --- 链接库 ---
target_link_libraries(generator PRIVATE nlohmann_json::nlohmann_json)

# --- 复制配置文件 ---
# *** 新增: 添加一个自定义目标，在编译时将 src/config 目录复制到输出目录 ***
add_custom_command(
    TARGET generator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config"
    "$<TARGET_FILE_DIR:generator>/config"
    COMMENT "Copying config directory to output bin folder"
)

# --- 可选的 Release 构建优化 ---
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang") 
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -march=native -flto")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")
    message(STATUS "GCC/Clang specific release optimizations enabled (-Os, -march=native, -flto, -s)")
endif()

# --- 用户反馈信息 ---
message(STATUS "Project configured successfully. Executable will be named 'generator'.")
message(STATUS "To build, run: cmake --build .")
message(STATUS "The output will be in the 'build/bin' directory.")