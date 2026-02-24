# Select compiler before project() if BILL_COMPILER is provided.
# Usage: cmake -DBILL_COMPILER=clang .. or cmake -DBILL_COMPILER=gcc ..

if(DEFINED BILL_COMPILER)
  string(TOLOWER "${BILL_COMPILER}" _bill_compiler)

  if(_bill_compiler STREQUAL "clang")
    set(_bill_c_compiler "clang")
    set(_bill_cxx_compiler "clang++")
  elseif(_bill_compiler STREQUAL "gcc")
    set(_bill_c_compiler "gcc")
    set(_bill_cxx_compiler "g++")
  else()
    message(FATAL_ERROR "BILL_COMPILER must be 'clang' or 'gcc'.")
  endif()

  if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER "${_bill_c_compiler}" CACHE STRING "Selected C compiler" FORCE)
  endif()
  if(NOT CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "${_bill_cxx_compiler}" CACHE STRING "Selected C++ compiler" FORCE)
  endif()
endif()
