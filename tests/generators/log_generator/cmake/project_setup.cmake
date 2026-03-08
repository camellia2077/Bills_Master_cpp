# Basic project paths and output directories.

set(SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Place executables in dist/cmake/.../bin under the configured build tree.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
