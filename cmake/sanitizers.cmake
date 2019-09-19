# only one of them can be enabled simultaneously
if(SANITIZE_ADDRESS)
    message(STATUS "SANITIZE_ADDRESS enabled")
    set(FLAGS
        -fsanitize=address
        -fsanitize-address-use-after-scope
        -g
        -O1
        -DNDEBUG
        )
    foreach(FLAG IN LISTS FLAGS)
        add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
        add_cache_flag(CMAKE_C_FLAGS ${FLAG})
    endforeach()

    set(_ENV "verbosity=1:debug=1:detect_leaks=1:check_initialization_order=1:alloc_dealloc_mismatch=true:use_odr_indicator=true")

    set(ENV{ASAN_OPTIONS} "${_ENV}")
    message(STATUS """
            [Warning] Define ASAN_OPTIONS ENV var:

            export ASAN_OPTIONS="${_ENV}"
    """)
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

    set(_ENV "verbosity=1 exitcode=32 suppressions=${CMAKE_CURRENT_LIST_DIR}/tsan_suppressions.txt")
    set(ENV{TSAN_OPTIONS} "${_ENV}")
    message(STATUS """
            [Warning] Define TSAN_OPTIONS ENV var:

            export TSAN_OPTIONS="${_ENV}"
    """)
elseif(SANITIZE_UNDEFINED)
    message(STATUS "SANITIZE_UNDEFINED enabled")

    set(FLAGS
        -fsanitize=undefined
        -fno-omit-frame-pointer
        -g
        )
    foreach(FLAG IN LISTS FLAGS)
        add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
        add_cache_flag(CMAKE_C_FLAGS ${FLAG})
    endforeach()

    set(ENV{UBSAN_OPTIONS} print_stacktrace=1)
    set(_ENV "print_stacktrace=1")
    set(ENV{UBSAN_OPTIONS} "${_ENV}")
    message(STATUS """
            [Warning] Define UBSAN_OPTIONS ENV var:

            export UBSAN_OPTIONS="${_ENV}"
    """)
endif()
