# Source file collection.

set(LOG_GENERATOR_SOURCES
    "${SOURCE_ROOT}/main.cpp"
    "${SOURCE_ROOT}/internal/bill_generator.cpp"
    "${SOURCE_ROOT}/internal/config_io.cpp"
    "${SOURCE_ROOT}/internal/file_io.cpp"
    "${SOURCE_ROOT}/presentation/cli_app.cpp"
)

# Align with shared tooling scripts.
set(SHARED_SOURCES
    ${LOG_GENERATOR_SOURCES}
)
