cmake_minimum_required(VERSION 3.28)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/modules/compiler_select.cmake")

project(BillsCore LANGUAGES CXX)

option(BILLS_ENABLE_LTO_RELEASE "Enable LTO for Release builds" ON)
option(BILLS_ENABLE_MODULES "Enable C++ modules pilot for bills_core" OFF)
option(BILLS_CORE_ENABLE_RENDERER_JSON "Enable JSON report renderer" ON)
option(BILLS_CORE_ENABLE_RENDERER_MD "Enable Markdown report renderer" ON)
option(BILLS_CORE_ENABLE_RENDERER_RST "Enable reStructuredText report renderer" ON)
option(BILLS_CORE_ENABLE_RENDERER_TEX "Enable LaTeX report renderer" ON)
option(BILLS_CORE_ENABLE_RENDERER_TYP "Enable Typst report renderer" ON)

if(BILLS_ENABLE_MODULES AND CMAKE_VERSION VERSION_LESS 3.28)
    message(FATAL_ERROR "BILLS_ENABLE_MODULES requires CMake >= 3.28")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32 AND MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Ensure Clang LTO links through lld on MinGW.
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fuse-ld=lld")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE
        "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -fuse-ld=lld")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE
        "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -fuse-ld=lld")
endif()

set(BILLS_CORE_CAN_USE_LTO TRUE)
if(WIN32 AND MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(BILLS_CORE_CAN_USE_LTO FALSE)
    message(WARNING
        "BILLS_ENABLE_LTO_RELEASE requested, but disabled for MinGW Clang "
        "because static release linking is not stable with LTO on this project."
    )
endif()

if(BILLS_ENABLE_LTO_RELEASE AND NOT MSVC AND BILLS_CORE_CAN_USE_LTO)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -flto")
endif()

set(OUTPUT_BINARY_DIR "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_BINARY_DIR})

option(BILLS_CORE_BUILD_SHARED "Build bills_core as a shared library" ON)
if(WIN32 AND BILLS_CORE_BUILD_SHARED)
    message(FATAL_ERROR
        "Windows no longer supports BILLS_CORE_BUILD_SHARED=ON. "
        "Use the static bills_core archive instead."
    )
endif()

set(CORE_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/src")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/source_files.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/modules/native_dependencies.cmake")

if(NOT TARGET nlohmann_json::nlohmann_json)
    find_package(nlohmann_json REQUIRED)
endif()

if(BILLS_CORE_BUILD_SHARED)
    set(BILLS_CORE_LIBRARY_TYPE SHARED)
else()
    set(BILLS_CORE_LIBRARY_TYPE STATIC)
endif()

add_library(bills_core ${BILLS_CORE_LIBRARY_TYPE}
    ${CORE_SOURCES}
)
add_library(bill_master_core ALIAS bills_core)

if(BILLS_ENABLE_MODULES)
    target_sources(bills_core PRIVATE
        ${CORE_MODULE_CONSUMER_SOURCES}
    )
    target_sources(bills_core PRIVATE
        FILE_SET CXX_MODULES FILES
            ${CORE_MODULE_INTERFACE_FILES}
    )
endif()

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
if(BILLS_CORE_ENABLE_RENDERER_JSON)
    target_compile_definitions(bills_core PUBLIC BILLS_CORE_ENABLE_RENDERER_JSON=1)
else()
    target_compile_definitions(bills_core PUBLIC BILLS_CORE_ENABLE_RENDERER_JSON=0)
endif()
if(BILLS_CORE_ENABLE_RENDERER_MD)
    target_compile_definitions(bills_core PUBLIC BILLS_CORE_ENABLE_RENDERER_MD=1)
else()
    target_compile_definitions(bills_core PUBLIC BILLS_CORE_ENABLE_RENDERER_MD=0)
endif()
if(BILLS_CORE_ENABLE_RENDERER_RST)
    target_compile_definitions(bills_core PUBLIC BILLS_CORE_ENABLE_RENDERER_RST=1)
else()
    target_compile_definitions(bills_core PUBLIC BILLS_CORE_ENABLE_RENDERER_RST=0)
endif()
if(BILLS_CORE_ENABLE_RENDERER_TEX)
    target_compile_definitions(bills_core PUBLIC BILLS_CORE_ENABLE_RENDERER_TEX=1)
else()
    target_compile_definitions(bills_core PUBLIC BILLS_CORE_ENABLE_RENDERER_TEX=0)
endif()
if(BILLS_CORE_ENABLE_RENDERER_TYP)
    target_compile_definitions(bills_core PUBLIC BILLS_CORE_ENABLE_RENDERER_TYP=1)
else()
    target_compile_definitions(bills_core PUBLIC BILLS_CORE_ENABLE_RENDERER_TYP=0)
endif()
if(BILLS_ENABLE_MODULES)
    target_compile_definitions(bills_core PUBLIC BILLS_CORE_MODULES_ENABLED=1)
else()
    target_compile_definitions(bills_core PUBLIC BILLS_CORE_MODULES_ENABLED=0)
endif()

target_link_libraries(bills_core PUBLIC
    nlohmann_json::nlohmann_json
    tomlplusplus::tomlplusplus
)
set(BILLS_STDCXXEXP_LIBRARY "")
if(WIN32 AND MINGW)
    set(BILLS_STDCXXEXP_LIBRARY "")
elseif(WIN32 AND NOT ANDROID)
    get_filename_component(BILLS_TOOLCHAIN_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(BILLS_TOOLCHAIN_ROOT "${BILLS_TOOLCHAIN_BIN_DIR}" DIRECTORY)
    find_library(
        BILLS_STDCXXEXP_LIBRARY
        stdc++exp
        HINTS "${BILLS_TOOLCHAIN_ROOT}/lib"
        NO_DEFAULT_PATH
    )
else()
    find_library(BILLS_STDCXXEXP_LIBRARY stdc++exp)
endif()
if(BILLS_STDCXXEXP_LIBRARY AND NOT ANDROID)
    target_link_libraries(bills_core PUBLIC ${BILLS_STDCXXEXP_LIBRARY})
endif()
target_compile_options(bills_core PRIVATE
    -Wall
    -fdiagnostics-color=always
)
