# cmake/AddFormatTarget.cmake

find_program(CLANG_FORMAT_EXE NAMES "clang-format")

if(CLANG_FORMAT_EXE)
    message(STATUS "Found clang-format: ${CLANG_FORMAT_EXE}")
    if(CMAKE_VERSION VERSION_LESS 3.12)
        find_package(PythonInterp REQUIRED)
        set(BILLS_PYTHON_EXECUTABLE "${PYTHON_EXECUTABLE}")
    else()
        find_package(Python3 COMPONENTS Interpreter REQUIRED)
        set(BILLS_PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")
    endif()

    get_filename_component(BILLS_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
    set(BILLS_TOOLS_ROOT "${BILLS_REPO_ROOT}/tools")

    execute_process(
        COMMAND "${BILLS_PYTHON_EXECUTABLE}" -m toolchain.scripts.scope_export --mode format-sources
        WORKING_DIRECTORY "${BILLS_TOOLS_ROOT}"
        OUTPUT_VARIABLE ALL_FORMAT_SOURCES_RAW
        ERROR_VARIABLE BILLS_FORMAT_SCOPE_ERROR
        RESULT_VARIABLE BILLS_FORMAT_SCOPE_RESULT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT BILLS_FORMAT_SCOPE_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to resolve format scope from workflow.toml: ${BILLS_FORMAT_SCOPE_ERROR}")
    endif()

    if(NOT ALL_FORMAT_SOURCES_RAW)
        add_custom_target(format
            COMMAND "${CMAKE_COMMAND}" -E echo "No clang-format sources resolved from tools/toolchain/config/workflow.toml."
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            VERBATIM
        )
        message(WARNING "No clang-format files resolved from workflow.toml.")
    else()
        set(ALL_FORMAT_SOURCES ${ALL_FORMAT_SOURCES_RAW})
        add_custom_target(format
            COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${ALL_FORMAT_SOURCES}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running clang-format on workflow.toml scoped source files..."
            VERBATIM
        )
    endif()
else()
    message(WARNING "clang-format not found. 'format' target will not be available.")
endif()
