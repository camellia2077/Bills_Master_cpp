include(FetchContent)

if(NOT TARGET nlohmann_json::nlohmann_json AND NOT (WIN32 AND MINGW))
    find_package(nlohmann_json QUIET)
endif()
if(NOT TARGET nlohmann_json::nlohmann_json)
    FetchContent_Declare(
        nlohmann_json
        URL "https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

if(NOT TARGET tomlplusplus::tomlplusplus)
    FetchContent_Declare(
        tomlplusplus
        URL "https://github.com/marzer/tomlplusplus/archive/refs/tags/v3.4.0.zip"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_GetProperties(tomlplusplus)
    if(NOT tomlplusplus_POPULATED)
        if(POLICY CMP0169)
            cmake_policy(PUSH)
            cmake_policy(SET CMP0169 OLD)
            FetchContent_Populate(tomlplusplus)
            cmake_policy(POP)
        else()
            FetchContent_Populate(tomlplusplus)
        endif()
    endif()

    add_library(bills_tomlplusplus INTERFACE)
    add_library(tomlplusplus::tomlplusplus ALIAS bills_tomlplusplus)
    target_include_directories(bills_tomlplusplus SYSTEM INTERFACE
        "${tomlplusplus_SOURCE_DIR}/include"
    )
    target_compile_definitions(bills_tomlplusplus INTERFACE
        TOML_HEADER_ONLY=1
        TOML_SHARED_LIB=0
    )
endif()

if(CMAKE_C_COMPILER_LOADED AND NOT TARGET sqlite3_amalgamation)
    FetchContent_Declare(
        sqlite_amalgamation
        URL "https://www.sqlite.org/2026/sqlite-amalgamation-3510200.zip"
        URL_HASH "SHA256=6E2A845A493026BDBAD0618B2B5A0CF48584FAAB47384480ED9F592D912F23EC"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(sqlite_amalgamation)

    add_library(sqlite3_amalgamation STATIC
        "${sqlite_amalgamation_SOURCE_DIR}/sqlite3.c"
    )
    target_include_directories(sqlite3_amalgamation PUBLIC
        "${sqlite_amalgamation_SOURCE_DIR}"
    )
    target_compile_definitions(sqlite3_amalgamation PRIVATE
        SQLITE_OMIT_LOAD_EXTENSION
        SQLITE_OMIT_SHARED_CACHE
        SQLITE_OMIT_DEPRECATED
        SQLITE_OMIT_PROGRESS_CALLBACK
        SQLITE_OMIT_UTF16
        SQLITE_OMIT_COMPLETE
        SQLITE_DEFAULT_MEMSTATUS=0
    )
    if(NOT MSVC)
        target_compile_options(sqlite3_amalgamation PRIVATE
            -ffunction-sections
            -fdata-sections
        )
    endif()
endif()

if(CMAKE_C_COMPILER_LOADED AND NOT TARGET miniz)
    FetchContent_Declare(
        miniz_external
        URL "https://github.com/richgel999/miniz/archive/refs/tags/3.1.0.zip"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(miniz_external)

    if(NOT TARGET miniz)
        add_library(miniz STATIC
            "${miniz_external_SOURCE_DIR}/miniz.c"
        )
        target_include_directories(miniz PUBLIC
            "${miniz_external_SOURCE_DIR}"
        )
        target_compile_definitions(miniz PRIVATE
            MINIZ_NO_ZLIB_COMPATIBLE_NAMES
        )
    endif()
endif()
