cmake_minimum_required(VERSION 3.10)

project(BillsIo LANGUAGES CXX)

if(NOT TARGET tomlplusplus::tomlplusplus)
    find_package(tomlplusplus CONFIG REQUIRED)
endif()

set(BILLS_IO_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/src")
set(BILLS_CORE_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../bills_core/src")

set(BILLS_IO_SOURCES
    "${BILLS_IO_SOURCE_ROOT}/bills_io/io_factory.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/io/file_bill_content_reader.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/io/file_bill_file_enumerator.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/config/toml_bill_config_loader.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/config/toml_config_provider.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/config/toml_modifier_config_loader.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/reports/builtin_month_report_formatter_provider.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/reports/builtin_yearly_report_formatter_provider.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/db/sqlite_bill_repository.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/db/bill_inserter.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/db/database_manager.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/db/month_query.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/db/year_query.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/db/sqlite_report_db_session.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/db/sqlite_report_data_gateway.cpp"
)

add_library(bills_io STATIC ${BILLS_IO_SOURCES})
add_library(bill_master_io ALIAS bills_io)

target_include_directories(bills_io PUBLIC
    "${BILLS_IO_SOURCE_ROOT}"
    "${BILLS_CORE_SOURCE_ROOT}"
)

if(TARGET sqlite3_amalgamation)
    target_link_libraries(bills_io PUBLIC sqlite3_amalgamation)
endif()
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(bills_io PUBLIC nlohmann_json::nlohmann_json)
endif()
if(TARGET tomlplusplus::tomlplusplus)
    target_link_libraries(bills_io PUBLIC tomlplusplus::tomlplusplus)
endif()

target_compile_features(bills_io PUBLIC cxx_std_23)
if(BILLS_FORMAT_COMPILE_DEFINITIONS)
    target_compile_definitions(bills_io PUBLIC ${BILLS_FORMAT_COMPILE_DEFINITIONS})
endif()

if(COMMON_COMPILE_OPTIONS)
    target_compile_options(bills_io PRIVATE ${COMMON_COMPILE_OPTIONS})
endif()

if(COMMON_LINK_OPTIONS)
    target_link_options(bills_io PRIVATE ${COMMON_LINK_OPTIONS})
endif()
