# External dependencies.
include("${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/modules/native_dependencies.cmake")
include(FetchContent)

if(NOT TARGET CLI11::CLI11)
    FetchContent_Declare(
        cli11_external
        URL "https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.6.1.zip"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(cli11_external)
endif()

set(COMMON_LINK_LIBRARIES
    tomlplusplus::tomlplusplus
    CLI11::CLI11
)
