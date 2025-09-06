# Minimum CMake version requirement. 3.12 is a good baseline for C++17 projects. 
cmake_minimum_required(VERSION 3.12)

# Define the project name and specify CXX (C++) as the language.
project(BillGenerator CXX) 

# --- Standard and Compiler Settings ---
# Set the C++ standard to C++17, which is required for std::filesystem.
set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Find Dependencies ---
# Find the nlohmann_json library using its modern CMake config-file package.
find_package(nlohmann_json 3.2.0 CONFIG REQUIRED) 

# --- Add Executable Target ---
# Create an executable named 'generator' from the specified source files.
# Absolute paths are used via the ${CMAKE_CURRENT_SOURCE_DIR} variable.
add_executable(generator
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/_internal/BillGenerator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/_internal/arg_parser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/_internal/utils.cpp"
)

# --- Include Directories ---
# Add the '_internal' directory to the include path for the 'generator' target.
# This allows files like main.cpp to find headers (e.g., #include "BillGenerator.h")
# that are located in the subdirectory.
target_include_directories(generator PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/_internal")

# --- Link Libraries ---
# Link the nlohmann_json library to our 'generator' executable. 
target_link_libraries(generator PRIVATE nlohmann_json::nlohmann_json)

# --- Optional: Add Release Build Optimizations for GCC/Clang ---
# These flags improve performance and reduce the size of the release build.
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang") 
    # -O3: High-level optimization. 
    # -march=native: Optimize for the CPU architecture of the build machine. 
    # -flto: Enable Link-Time Optimization for cross-file optimizations.
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -march=native -flto")

    # -s: Strip the executable of symbol information to reduce file size.
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")

    message(STATUS "GCC/Clang specific release optimizations enabled (-O3, -march=native, -flto, -s)")
endif()

# --- Installation (Optional) ---
# Defines a simple way to install the executable.
install(TARGETS generator
    DESTINATION bin
)

# --- User Feedback ---
message(STATUS "Project configured successfully. Executable will be named 'generator'.")
message(STATUS "To build, run: cmake --build .")
message(STATUS "To build for release with optimizations, run: cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build .")