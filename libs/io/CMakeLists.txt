cmake_minimum_required(VERSION 3.10)

project(BillsIo LANGUAGES C CXX)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/modules/native_dependencies.cmake")

set(BILLS_IO_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/src")
set(BILLS_CORE_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../bills_core/src")

set(BILLS_IO_SOURCES
    "${BILLS_IO_SOURCE_ROOT}/io/io_factory.cpp"
    "${BILLS_IO_SOURCE_ROOT}/io/host_flow_support.cpp"
    "${BILLS_IO_SOURCE_ROOT}/io/adapters/config/config_document_parser.cpp"
    "${BILLS_IO_SOURCE_ROOT}/io/adapters/io/year_partition_output_path_builder.cpp"
    "${BILLS_IO_SOURCE_ROOT}/io/adapters/io/source_document_io.cpp"
    "${BILLS_IO_SOURCE_ROOT}/io/adapters/io/zip_archive_io.cpp"
    "${BILLS_IO_SOURCE_ROOT}/io/adapters/io/json_bill_document_io.cpp"
    "${BILLS_IO_SOURCE_ROOT}/io/adapters/reports/report_export_service.cpp"
    "${BILLS_IO_SOURCE_ROOT}/io/adapters/db/sqlite_bill_repository.cpp"
    "${BILLS_IO_SOURCE_ROOT}/io/adapters/db/bill_inserter.cpp"
    "${BILLS_IO_SOURCE_ROOT}/io/adapters/db/database_manager.cpp"
    "${BILLS_IO_SOURCE_ROOT}/io/adapters/db/month_query.cpp"
    "${BILLS_IO_SOURCE_ROOT}/io/adapters/db/year_query.cpp"
    "${BILLS_IO_SOURCE_ROOT}/io/adapters/db/sqlite_report_db_session.cpp"
    "${BILLS_IO_SOURCE_ROOT}/io/adapters/db/sqlite_report_data_gateway.cpp"
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
if(TARGET miniz)
    target_link_libraries(bills_io PUBLIC miniz)
endif()

target_compile_features(bills_io PUBLIC cxx_std_23)

if(COMMON_COMPILE_OPTIONS)
    target_compile_options(bills_io PRIVATE ${COMMON_COMPILE_OPTIONS})
endif()

if(COMMON_LINK_OPTIONS)
    target_link_options(bills_io PRIVATE ${COMMON_LINK_OPTIONS})
endif()
