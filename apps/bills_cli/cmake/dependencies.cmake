include("${CMAKE_CURRENT_LIST_DIR}/../../../cmake/modules/native_dependencies.cmake")

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
