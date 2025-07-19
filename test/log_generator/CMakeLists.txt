# CMakeLists.txt 

# 最小CMake版本要求 
cmake_minimum_required(VERSION 3.10) 

# 定义项目名称 
project(BillGenerator CXX) 

# 设置C++标准为C++17，因为你使用了<filesystem> 
set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
set(CMAKE_CXX_EXTENSIONS OFF) 

# --- Release Build Optimizations (GCC/Clang) ---
# The following flags are applied only for Release builds
# (e.g., when running `cmake .. -DCMAKE_BUILD_TYPE=Release`)
# and only for GCC and Clang compilers.
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # -O3: The highest level of optimization. Note: This is a capital 'O', not a zero.
    # -march=native: Optimizes the code for the specific CPU architecture of the machine you are compiling on.
    # -flto: Enables Link-Time Optimization, allowing for optimizations across different source files.
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -flto")

    # -s: Strips the executable of symbol information, significantly reducing its file size.
    # This is a linker flag.
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")

    message(STATUS "Enabled GCC/Clang specific release optimizations: -O3, -march=native, -flto, -s")
endif()


# 查找 nlohmann/json 库 
# 使用 CONFIG 模式查找，这是查找通过包管理器安装的库的推荐方式 
# nlohmann_json 的 CMake 配置通常允许直接找到它 
find_package(nlohmann_json CONFIG REQUIRED) 

# 添加可执行文件 
# generator.cpp 是你的源文件 
add_executable(generator generator.cpp) 

# 将 nlohmann_json 库链接到你的可执行文件 
# nlohmann_json::nlohmann_json 是 nlohmann/json 提供的目标名称 
target_link_libraries(generator PRIVATE nlohmann_json::nlohmann_json) 