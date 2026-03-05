cmake_minimum_required(VERSION 3.10)

project(BillsIo LANGUAGES CXX)

set(BILLS_IO_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/src")
set(BILLS_CORE_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../bills_core/src")

set(BILLS_IO_SOURCES
    "${BILLS_IO_SOURCE_ROOT}/bills_io/io_factory.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/io/file_bill_content_reader.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/io/file_bill_file_enumerator.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/config/json_bill_config_loader.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/config/json_config_provider.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/config/json_modifier_config_loader.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/plugins/common/dynamic_month_report_formatter_provider.cpp"
    "${BILLS_IO_SOURCE_ROOT}/bills_io/adapters/plugins/common/dynamic_yearly_report_formatter_provider.cpp"
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

target_compile_features(bills_io PUBLIC cxx_std_23)

if(COMMON_COMPILE_OPTIONS)
    target_compile_options(bills_io PRIVATE ${COMMON_COMPILE_OPTIONS})
endif()

if(COMMON_LINK_OPTIONS)
    target_link_options(bills_io PRIVATE ${COMMON_LINK_OPTIONS})
endif()
