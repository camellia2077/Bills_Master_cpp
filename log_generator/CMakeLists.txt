# CMakeLists.txt

# 最小CMake版本要求
cmake_minimum_required(VERSION 3.10)

# 定义项目名称
project(BillGenerator CXX)

# 设置C++标准为C++17，因为你使用了<filesystem>
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

# 如果你的 generator.cpp 中仍然使用 #include "json.hpp"
# 并且你希望编译器能找到它，你可能需要手动添加其父目录到include路径
# 但如果你已经改为 #include <nlohmann/json/json.hpp> 或者 #include <nlohmann/json.hpp>
# 并且 find_package 已经设置好了，则不需要以下行。
# 例如，如果json.hpp在 /mingw64/include/nlohmann/json/json.hpp
# 那么 find_package 通常会添加 /mingw64/include 到头文件搜索路径
# 并且你需要在代码中使用 #include <nlohmann/json/json.hpp>
# 如果 find_package 无法正确设置，或者你需要更精确的控制，可以取消注释以下行：
# target_include_directories(generator PRIVATE ${CMAKE_SOURCE_DIR}) # 如果json.hpp在项目根目录
# target_include_directories(generator PRIVATE /mingw64/include) # 假设json.hpp在 /mingw64/include/nlohmann/json/json.hpp
