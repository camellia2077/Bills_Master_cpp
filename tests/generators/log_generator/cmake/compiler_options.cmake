# C++ standard and compile database for clang-tidy.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include("${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/modules/windows_static_runtime.cmake")

if(WIN32 AND MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # Use lld for MinGW+Clang to avoid noisy GNU ld duplicate section warnings.
  foreach(_bills_flag_var
      CMAKE_C_FLAGS
      CMAKE_CXX_FLAGS
      CMAKE_EXE_LINKER_FLAGS
      CMAKE_SHARED_LINKER_FLAGS
      CMAKE_MODULE_LINKER_FLAGS
  )
    if(NOT "${${_bills_flag_var}}" MATCHES "(^| )-fuse-ld=lld($| )")
      set(${_bills_flag_var} "${${_bills_flag_var}} -fuse-ld=lld")
    endif()
  endforeach()
endif()

# Release optimization flags (mirrors existing behavior).
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -march=native -flto")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")
  message(STATUS "GCC/Clang specific release optimizations enabled (-Os, -march=native, -flto, -s)")
endif()
