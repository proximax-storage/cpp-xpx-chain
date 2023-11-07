# only one of them can be enabled simultaneously
if(SANITIZE_ADDRESS)
    message(STATUS "SANITIZE_ADDRESS enabled")
    set(FLAGS
        -fsanitize=address
        -fsanitize-address-use-after-scope
        -g
        -O1
        -DNDEBUG
        -fsanitize-blacklist=${CMAKE_CURRENT_LIST_DIR}/asan_suppressions.txt
        )
    foreach(FLAG IN LISTS FLAGS)
        add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
        add_cache_flag(CMAKE_C_FLAGS ${FLAG})
    endforeach()

    set(_ENV "suppressions=${CMAKE_CURRENT_LIST_DIR}/asan_suppressions.txt:verbosity=1:debug=1:detect_leaks=1:check_initialization_order=1:alloc_dealloc_mismatch=true:use_odr_indicator=true")

    set(ENV{ASAN_OPTIONS} "${_ENV}")
    message(STATUS "
    [Warning] Define ASAN_OPTIONS ENV var:

    export ASAN_OPTIONS=\"${_ENV}\"
    ")
elseif(SANITIZE_THREAD)
    message(STATUS "SANITIZE_THREAD enabled")

    set(BLACKLIST -fsanitize-blacklist=${CMAKE_CURRENT_LIST_DIR}/tsan_blacklist.txt)
    set(FLAGS
            ${BLACKLIST}
            -gcolumn-info
            -g
            -fsanitize=thread
            -DNDEBUG
            )
    foreach(FLAG IN LISTS FLAGS)
        add_cache_flag(CMAKE_CXX_FLAGS ${BLACKLIST} ${FLAG})
        add_cache_flag(CMAKE_C_FLAGS ${BLACKLIST} ${FLAG})
    endforeach()

    add_cache_flag(CMAKE_EXE_LINKER_FLAGS "-fsanitize=thread")
    add_cache_flag(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=thread")

    set(_ENV "verbosity=1:exitcode=32:suppressions=${CMAKE_CURRENT_LIST_DIR}/tsan_suppressions.txt")
    set(ENV{TSAN_OPTIONS} "${_ENV}")
    message(STATUS "
    [Warning] Define TSAN_OPTIONS ENV var:

    export TSAN_OPTIONS=\"${_ENV}\"
    ")
elseif(SANITIZE_UNDEFINED)
    message(STATUS "SANITIZE_UNDEFINED enabled")

    set(FLAGS
        -fsanitize=undefined
        -fno-omit-frame-pointer
        -g
        -O1
        )
    foreach(FLAG IN LISTS FLAGS)
        add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
        add_cache_flag(CMAKE_C_FLAGS ${FLAG})
    endforeach()

    set(_ENV "print_stacktrace=1")
    set(ENV{UBSAN_OPTIONS} "${_ENV}")
    message(STATUS "
    [Warning] Define UBSAN_OPTIONS ENV var:

    export UBSAN_OPTIONS=\"${_ENV}\"
    ")
elseif(SANITIZE_MEMORY)
    message(STATUS "SANITIZE_MEMORY enabled")

    set(_ENV "halt_on_error=1:report_umrs=1:suppressions=${CMAKE_CURRENT_LIST_DIR}/msan_ignorelist.txt")
    set(ENV{MSAN_OPTIONS} "${_ENV}")
    set(FLAGS
        -fsanitize=memory
        -fPIE
        -fno-omit-frame-pointer
        -g
        -fno-optimize-sibling-calls
        -O1
        -fsanitize-memory-track-origins=2
        -fsanitize-memory-track-origins=${MSAN_OPTIONS}
        )
    foreach(FLAG IN LISTS FLAGS)
        add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
        add_cache_flag(CMAKE_C_FLAGS ${FLAG})
    endforeach()

    message(STATUS "
    [Warning] Define MSAN_OPTIONS ENV var:

    export MSAN_OPTIONS=\"${_ENV}\"
    ")
endif()
