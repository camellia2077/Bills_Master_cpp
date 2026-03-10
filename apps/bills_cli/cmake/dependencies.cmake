include("${CMAKE_CURRENT_LIST_DIR}/../../../cmake/modules/native_dependencies.cmake")

# 定义通用的链接库
set(COMMON_LINK_LIBRARIES
    nlohmann_json::nlohmann_json
    tomlplusplus::tomlplusplus
    stdc++exp
)

# CLI 额外依赖（不下沉到 core）
set(CLI_LINK_LIBRARIES)
