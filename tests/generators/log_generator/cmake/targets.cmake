# Executable target.
add_executable(generator
    ${LOG_GENERATOR_SOURCES}
)

target_include_directories(generator PRIVATE "${SOURCE_ROOT}/internal")
target_link_libraries(generator PRIVATE ${COMMON_LINK_LIBRARIES})

# Copy config folder to output directory.
add_custom_command(
    TARGET generator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${SOURCE_ROOT}/config"
    "$<TARGET_FILE_DIR:generator>/config"
    COMMENT "Copying config directory to output bin folder"
)
