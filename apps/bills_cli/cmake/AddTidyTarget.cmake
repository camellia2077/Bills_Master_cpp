# cmake/AddTidyTarget.cmake

find_program(CLANG_TIDY_EXE NAMES "clang-tidy")

if(CLANG_TIDY_EXE)
    message(STATUS "Found clang-tidy for checks: ${CLANG_TIDY_EXE}")
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
        COMMAND "${BILLS_PYTHON_EXECUTABLE}" -m toolchain.scripts.scope_export --mode header-filter
        WORKING_DIRECTORY "${BILLS_TOOLS_ROOT}"
        OUTPUT_VARIABLE TIDY_HEADER_FILTER
        ERROR_VARIABLE BILLS_TIDY_FILTER_ERROR
        RESULT_VARIABLE BILLS_TIDY_FILTER_RESULT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT BILLS_TIDY_FILTER_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to resolve tidy header filter from workflow.toml: ${BILLS_TIDY_FILTER_ERROR}")
    endif()

    set(BILLS_TIDY_SOURCE_TARGETS
        bills_core
        bills_io
        bills_tracer_cli
    )
    set(ALL_TIDY_SOURCES)
    foreach(BILLS_TIDY_TARGET ${BILLS_TIDY_SOURCE_TARGETS})
        if(NOT TARGET ${BILLS_TIDY_TARGET})
            continue()
        endif()

        get_target_property(BILLS_TIDY_TARGET_SOURCES ${BILLS_TIDY_TARGET} SOURCES)
        if(NOT BILLS_TIDY_TARGET_SOURCES)
            continue()
        endif()

        foreach(FILE_PATH ${BILLS_TIDY_TARGET_SOURCES})
            get_filename_component(FILE_EXT "${FILE_PATH}" EXT)
            string(TOLOWER "${FILE_EXT}" FILE_EXT_LOWER)
            if(NOT FILE_EXT_LOWER MATCHES "^\\.(cpp|cc|cxx)$")
                continue()
            endif()

            get_filename_component(
                FILE_PATH_ABSOLUTE
                "${FILE_PATH}"
                ABSOLUTE
                BASE_DIR "${CMAKE_SOURCE_DIR}"
            )

            if(EXISTS "${FILE_PATH_ABSOLUTE}")
                get_filename_component(FILE_PATH_NORMALIZED "${FILE_PATH_ABSOLUTE}" REALPATH)
                list(APPEND ALL_TIDY_SOURCES "${FILE_PATH_NORMALIZED}")
            endif()
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES ALL_TIDY_SOURCES)

    if(NOT ALL_TIDY_SOURCES)
        add_custom_target(tidy
            COMMAND "${CMAKE_COMMAND}" -E echo "No clang-tidy compile units resolved from configured CMake targets."
        )
        message(WARNING "No clang-tidy compile units resolved from configured CMake targets.")
    else()
        add_custom_target(tidy)
        list(LENGTH ALL_TIDY_SOURCES TOTAL_TIDY_SOURCES)
        set(COUNTER 0)
        file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}" TIDY_BUILD_DIR_NATIVE)

        foreach(FILE_PATH ${ALL_TIDY_SOURCES})
            math(EXPR COUNTER "${COUNTER} + 1")
            set(CURRENT_TARGET "tidy_step_${COUNTER}")
            file(TO_NATIVE_PATH "${FILE_PATH}" FILE_PATH_NATIVE)

            add_custom_target(${CURRENT_TARGET}
                COMMAND ${CLANG_TIDY_EXE}
                    -p "${TIDY_BUILD_DIR_NATIVE}"
                    -header-filter=${TIDY_HEADER_FILTER}
                    "${FILE_PATH_NATIVE}"
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "[${COUNTER}/${TOTAL_TIDY_SOURCES}] Analyzing: ${FILE_PATH_NATIVE}"
                VERBATIM
            )

            add_dependencies(tidy ${CURRENT_TARGET})
        endforeach()

        if(COUNTER GREATER 0)
            message(STATUS "Configured tidy targets for ${COUNTER} compile units from configured CMake targets.")
        endif()
    endif()
else()
    message(WARNING "clang-tidy not found. 'tidy' target will not be available.")
endif()
