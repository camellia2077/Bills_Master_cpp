# cmake/AddFormatTarget.cmake

# 1. 查找工具
find_program(CLANG_FORMAT_EXE NAMES "clang-format")

if(CLANG_FORMAT_EXE)
    message(STATUS "Found clang-format: ${CLANG_FORMAT_EXE}")

    # 2. 自动扫描所有头文件
    file(GLOB_RECURSE ALL_HEADERS
        "${SOURCE_ROOT}/*.hpp"
        "${SOURCE_ROOT}/*.h"
    )

    # 3. 汇总所有需要格式化的文件
    set(ALL_FORMAT_SOURCES
        ${ALL_HEADERS}
        ${SHARED_SOURCES}
        "${SOURCE_ROOT}/main_command.cpp"
    )

    # 4. 定义自定义目标 'format'
    # 使用 -style=file 让 clang-format 读取项目根目录下的 .clang-format 配置文件
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${ALL_FORMAT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-format on all source (.cpp) and header (.hpp) files..."
        VERBATIM
    )
else()
    message(WARNING "clang-format not found. 'format' target will not be available.")
endif()
