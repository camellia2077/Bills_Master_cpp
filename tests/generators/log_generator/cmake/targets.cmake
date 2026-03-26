# Executable target.
add_executable(generator
    ${LOG_GENERATOR_SOURCES}
)

target_include_directories(generator PRIVATE "${SOURCE_ROOT}/internal")
target_include_directories(generator PRIVATE "${SOURCE_ROOT}")
target_link_libraries(generator PRIVATE ${COMMON_LINK_LIBRARIES})

# Copy config folder to output directory.
add_custom_command(
    TARGET generator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory
    "$<TARGET_FILE_DIR:generator>/config"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${SOURCE_ROOT}/config"
    "$<TARGET_FILE_DIR:generator>/config"
    COMMENT "Refreshing config directory in output bin folder"
)
