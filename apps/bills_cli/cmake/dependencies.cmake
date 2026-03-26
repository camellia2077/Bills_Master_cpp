include("${CMAKE_CURRENT_LIST_DIR}/../../../cmake/modules/native_dependencies.cmake")
include(FetchContent)

set(BILLS_CLI_STDCXXEXP_LIBRARY "")
if(WIN32 AND MINGW)
    set(BILLS_CLI_STDCXXEXP_LIBRARY "")
endif()

# 定义通用的链接库
set(COMMON_LINK_LIBRARIES
    nlohmann_json::nlohmann_json
    tomlplusplus::tomlplusplus
)
if(BILLS_CLI_STDCXXEXP_LIBRARY)
    list(APPEND COMMON_LINK_LIBRARIES "${BILLS_CLI_STDCXXEXP_LIBRARY}")
endif()

# CLI 额外依赖（不下沉到 core）
set(CLI_LINK_LIBRARIES)

if(NOT TARGET CLI11::CLI11)
    FetchContent_Declare(
        cli11_external
        URL "https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.6.1.zip"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(cli11_external)
endif()

list(APPEND CLI_LINK_LIBRARIES CLI11::CLI11)
