# --- 查找外部依赖 ---
find_package(nlohmann_json REQUIRED)

include(FetchContent)

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

# 定义通用的链接库
set(COMMON_LINK_LIBRARIES
    nlohmann_json::nlohmann_json
    stdc++exp
)

# CLI 额外依赖（不下沉到 core）
set(CLI_LINK_LIBRARIES)
