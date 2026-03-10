cmake_minimum_required(VERSION 3.22.1)

project(BillsAndroidNative LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(BILLS_ENABLE_LTO_RELEASE OFF CACHE BOOL "" FORCE)
set(BILLS_ENABLE_MODULES OFF CACHE BOOL "" FORCE)
set(BILLS_CORE_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(BILLS_CORE_ENABLE_RENDERER_JSON ON CACHE BOOL "" FORCE)
set(BILLS_CORE_ENABLE_RENDERER_MD ON CACHE BOOL "" FORCE)
set(BILLS_CORE_ENABLE_RENDERER_RST OFF CACHE BOOL "" FORCE)
set(BILLS_CORE_ENABLE_RENDERER_TEX OFF CACHE BOOL "" FORCE)
set(BILLS_CORE_ENABLE_RENDERER_TYP OFF CACHE BOOL "" FORCE)

set(BILLS_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../")

include("${BILLS_REPO_ROOT}/cmake/modules/native_dependencies.cmake")

add_subdirectory(
    "${BILLS_REPO_ROOT}/libs/bills_core"
    "${CMAKE_CURRENT_BINARY_DIR}/libs/bills_core"
)
add_subdirectory(
    "${BILLS_REPO_ROOT}/libs/bills_io"
    "${CMAKE_CURRENT_BINARY_DIR}/libs/bills_io"
)

add_library(bills_android_native SHARED
    android_bridge.cpp
)

find_library(ANDROID_LIB android)
find_library(LOG_LIB log)

target_include_directories(bills_android_native PRIVATE
    "${BILLS_REPO_ROOT}/libs/bills_core/src"
    "${BILLS_REPO_ROOT}/libs/bills_io/src"
)

target_link_libraries(bills_android_native PRIVATE
    bills_core
    bills_io
    ${ANDROID_LIB}
    ${LOG_LIB}
)

if(NOT MSVC)
    target_compile_options(bills_android_native PRIVATE
        -Wall
        -Wextra
        -fdiagnostics-color=always
    )
endif()
