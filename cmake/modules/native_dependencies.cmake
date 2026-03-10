include(FetchContent)

if(NOT TARGET nlohmann_json::nlohmann_json)
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
    find_package(tomlplusplus CONFIG QUIET)
endif()
if(NOT TARGET tomlplusplus::tomlplusplus)
    FetchContent_Declare(
        tomlplusplus
        URL "https://github.com/marzer/tomlplusplus/archive/refs/tags/v3.4.0.zip"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(tomlplusplus)
endif()

if(NOT TARGET sqlite3_amalgamation)
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
