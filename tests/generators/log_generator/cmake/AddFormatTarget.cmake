# cmake/AddFormatTarget.cmake

find_program(CLANG_FORMAT_EXE NAMES "clang-format")

if(CLANG_FORMAT_EXE)
  message(STATUS "Found clang-format: ${CLANG_FORMAT_EXE}")

  file(GLOB_RECURSE ALL_HEADERS
      "${SOURCE_ROOT}/*.hpp"
      "${SOURCE_ROOT}/*.h"
  )

  set(ALL_FORMAT_SOURCES
      ${ALL_HEADERS}
      ${SHARED_SOURCES}
  )

  add_custom_target(format
      COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${ALL_FORMAT_SOURCES}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Running clang-format on all source (.cpp) and header (.h/.hpp) files..."
      VERBATIM
  )
else()
  message(WARNING "clang-format not found. 'format' target will not be available.")
endif()
