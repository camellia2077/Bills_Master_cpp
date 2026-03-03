# Source file collection.

set(LOG_GENERATOR_SOURCES
    "${SOURCE_ROOT}/main.cpp"
    "${SOURCE_ROOT}/_internal/BillGenerator.cpp"
    "${SOURCE_ROOT}/_internal/arg_parser.cpp"
    "${SOURCE_ROOT}/_internal/utils.cpp"
)

# Align with shared tooling scripts.
set(SHARED_SOURCES
    ${LOG_GENERATOR_SOURCES}
)
