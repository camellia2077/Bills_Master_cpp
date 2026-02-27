cmake_minimum_required(VERSION 3.10)

project(BillsCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(OUTPUT_BINARY_DIR "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_BINARY_DIR})

option(BILLS_CORE_BUILD_SHARED "Build bills_core as a shared library" ON)

set(CORE_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/src")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/source_files.cmake")

find_package(nlohmann_json REQUIRED)

if(BILLS_CORE_BUILD_SHARED)
    set(BILLS_CORE_LIBRARY_TYPE SHARED)
else()
    set(BILLS_CORE_LIBRARY_TYPE STATIC)
endif()

add_library(bills_core ${BILLS_CORE_LIBRARY_TYPE}
    ${CORE_SOURCES}
)
add_library(bill_master_core ALIAS bills_core)

set_target_properties(bills_core PROPERTIES
    OUTPUT_NAME "bills_core"
)

if(BILLS_CORE_BUILD_SHARED AND WIN32)
    set_target_properties(bills_core PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        PREFIX ""
    )
endif()

target_include_directories(bills_core PUBLIC
    "${CORE_SOURCE_ROOT}"
)

target_compile_definitions(bills_core PRIVATE BILLS_CORE_ABI_BUILD)
if(BILLS_CORE_BUILD_SHARED)
    target_compile_definitions(bills_core PUBLIC BILLS_CORE_SHARED)
endif()

target_link_libraries(bills_core PUBLIC
    nlohmann_json::nlohmann_json
    stdc++exp
)
target_compile_options(bills_core PRIVATE
    -Wall
    -fdiagnostics-color=always
)
