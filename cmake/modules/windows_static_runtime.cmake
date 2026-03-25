include_guard(GLOBAL)

if(WIN32 AND MINGW AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(_BILLS_WINDOWS_STATIC_EXE_FLAGS
        -static
        -static-libgcc
        -static-libstdc++
        -Wl,-Bstatic
        -lwinpthread
    )

    foreach(_bills_flag_var
        CMAKE_EXE_LINKER_FLAGS
        CMAKE_MODULE_LINKER_FLAGS
    )
        foreach(_bills_static_flag IN LISTS _BILLS_WINDOWS_STATIC_EXE_FLAGS)
            string(FIND " ${${_bills_flag_var}} " " ${_bills_static_flag} " _bills_flag_index)
            if(_bills_flag_index EQUAL -1)
                set(${_bills_flag_var} "${${_bills_flag_var}} ${_bills_static_flag}")
            endif()
        endforeach()
    endforeach()
endif()
