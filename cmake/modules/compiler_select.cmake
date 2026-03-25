# Select compiler before project().
# The repository supports clang/clang++ only.

set(_bill_compiler "clang")
if (DEFINED BILL_COMPILER)
  string(TOLOWER "${BILL_COMPILER}" _bill_compiler)
endif()

if (NOT _bill_compiler STREQUAL "clang")
  message(FATAL_ERROR "BILL_COMPILER must be 'clang'. GCC is no longer supported.")
endif()
set(_bill_c_compiler "clang")
set(_bill_cxx_compiler "clang++")

if (WIN32)
  set(_bill_msys_mingw_root "C:/msys64/mingw64/bin")
  set(_bill_c_compiler "${_bill_msys_mingw_root}/${_bill_c_compiler}.exe")
  set(_bill_cxx_compiler "${_bill_msys_mingw_root}/${_bill_cxx_compiler}.exe")
  set(_bill_archiver "${_bill_msys_mingw_root}/ar.exe")
  set(_bill_ranlib "${_bill_msys_mingw_root}/ranlib.exe")
  if (NOT EXISTS "${_bill_c_compiler}" OR NOT EXISTS "${_bill_cxx_compiler}")
    message(FATAL_ERROR
      "Windows native static builds require the MSYS2 mingw64 toolchain at "
      "${_bill_msys_mingw_root}. Install the requested compiler there and do "
      "not fall back to ucrt64."
    )
  endif()
  if (NOT EXISTS "${_bill_archiver}" OR NOT EXISTS "${_bill_ranlib}")
    message(FATAL_ERROR
      "Windows native static builds require mingw64 binutils at "
      "${_bill_msys_mingw_root}."
    )
  endif()
endif()

set(CMAKE_C_COMPILER "${_bill_c_compiler}" CACHE STRING "Selected C compiler" FORCE)
set(CMAKE_CXX_COMPILER "${_bill_cxx_compiler}" CACHE STRING "Selected C++ compiler" FORCE)
if (WIN32)
  set(CMAKE_AR "${_bill_archiver}" CACHE FILEPATH "Selected archiver" FORCE)
  set(CMAKE_RANLIB "${_bill_ranlib}" CACHE FILEPATH "Selected ranlib" FORCE)
endif()
