# --- 查找外部依赖 ---
find_package(nlohmann_json REQUIRED)
set(SQLite3_USE_STATIC_LIBS OFF)
find_package(SQLite3 REQUIRED)

# 定义通用的链接库
set(COMMON_LINK_LIBRARIES
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
    stdc++exp
)