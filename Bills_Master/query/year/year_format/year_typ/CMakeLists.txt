# CMakeLists.txt for the Typst Yearly Formatter Plugin

# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Define the project name
project(YearTypFormatterPlugin CXX)

# Store the target name of the DLL plugin in a variable
set(PLUGIN_TARGET_NAME "typ_year_formatter")

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Define the plugin target ---
# Create the library using the ${PLUGIN_TARGET_NAME} variable
add_library(${PLUGIN_TARGET_NAME} SHARED
    YearTypFormat.cpp
)

# --- Configure include directories ---
# Ensure the compiler can find IYearlyReportFormatter.h and other headers
target_include_directories(${PLUGIN_TARGET_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

# --- Add compile and link optimization options for Release builds ---
target_compile_options(${PLUGIN_TARGET_NAME} PRIVATE
    $<$<CONFIG:Release>:-Os -flto -fvisibility=hidden -ffunction-sections -fdata-sections>
)

target_link_options(${PLUGIN_TARGET_NAME} PRIVATE
    $<$<CONFIG:Release>:-Wl,--gc-sections -s>
)

# --- Set plugin properties ---
# Remove the "lib" prefix from the library name on non-Windows platforms
# to ensure a consistent filename (e.g., typst_year_formatter.dll)
set_target_properties(${PLUGIN_TARGET_NAME} PROPERTIES
    PREFIX ""
)

# Print a status message
message(STATUS "Configured Typst yearly formatter plugin '${PLUGIN_TARGET_NAME}'.")