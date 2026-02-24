# External dependencies.
find_package(nlohmann_json 3.2.0 CONFIG REQUIRED)

set(COMMON_LINK_LIBRARIES
    nlohmann_json::nlohmann_json
)
