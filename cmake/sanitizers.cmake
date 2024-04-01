# only one of them can be enabled simultaneously
if(SANITIZE_ADDRESS)
    message(STATUS "SANITIZE_ADDRESS enabled")

    set(IGNORELIST -fsanitize-ignorelist=${CMAKE_CURRENT_LIST_DIR}/asan_ignorelist.txt)
    set(FLAGS
        ${IGNORELIST}
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
    message(STATUS "
    [Warning] Define ASAN_OPTIONS ENV var:

    export ASAN_OPTIONS=\"${_ENV}\"
    ")
elseif(SANITIZE_THREAD)
    message(STATUS "SANITIZE_THREAD enabled")

    set(BLACKLIST -fsanitize-blacklist=${CMAKE_CURRENT_LIST_DIR}/tsan_blacklist.txt)
    set(FLAGS
            -tsan-instrument-memory-accesses
            -mllvm
            ${BLACKLIST}
            -gcolumn-info
            -g
            -fsanitize=thread
            -DNDEBUG
            )
    foreach(FLAG IN LISTS FLAGS)
        add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
        add_cache_flag(CMAKE_C_FLAGS ${FLAG})
    endforeach()

    add_cache_flag(CMAKE_EXE_LINKER_FLAGS "-fsanitize=thread")
    add_cache_flag(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=thread")

    set(_ENV "suppressions=${CMAKE_CURRENT_LIST_DIR}/tsan_suppressions.txt verbosity=1 exitcode=32")
    set(ENV{TSAN_OPTIONS} "${_ENV}")
    message(STATUS "
    [Warning] Define TSAN_OPTIONS ENV var:

    export TSAN_OPTIONS=\"${_ENV}\"
    ")
elseif(SANITIZE_UNDEFINED)
    # if thre are issues with rocksDb try to re-build rocksDb lib with RTTI:
    # make EXTRA_CXXFLAGS=-fPIC EXTRA_CFLAGS=-fPIC USE_RTTI=1 DEBUG_LEVEL=0
    message(STATUS "SANITIZE_UNDEFINED enabled")

    set(IGNORELIST -fsanitize-ignorelist=${CMAKE_CURRENT_LIST_DIR}/ubsan_ignorelist.txt)
    set(FLAGS
        ${IGNORELIST}
        -fsanitize=undefined
        -fno-omit-frame-pointer
        -g
        -O1
        )
    foreach(FLAG IN LISTS FLAGS)
        add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
        add_cache_flag(CMAKE_C_FLAGS ${FLAG})
    endforeach()

    set(_ENV "print_stacktrace=1:suppressions=${CMAKE_CURRENT_LIST_DIR}/ubsan_ignorelist.txt")
    set(ENV{UBSAN_OPTIONS} "${_ENV}")
    message(STATUS "
    [Warning] Define UBSAN_OPTIONS ENV var:

    export UBSAN_OPTIONS=\"${_ENV}\"
    ")
elseif(SANITIZE_MEMORY)
    # some c++ std libs and google tests should be rebuild with MemorySanitizer too
    # see https://github.com/google/sanitizers/wiki/MemorySanitizerLibcxxHowTo
    message(STATUS "SANITIZE_MEMORY enabled")

    set(IGNORELIST -fsanitize-ignorelist=${CMAKE_CURRENT_LIST_DIR}/msan_ignorelist.txt)
    set(FLAGS
        ${IGNORELIST}
        -fsanitize=memory
        -fPIE
        -fno-omit-frame-pointer
        -g
        -fno-optimize-sibling-calls
        -O1
        -fsanitize-memory-track-origins=2
        -fsanitize-memory-use-after-dtor
        -fsanitize-undefined-trap-on-error
    )
    foreach(FLAG IN LISTS FLAGS)
        add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
        add_cache_flag(CMAKE_C_FLAGS ${FLAG})
    endforeach()

    set(_ENV "poison_in_dtor=0:halt_on_error=1:report_umrs=1:suppressions=${CMAKE_CURRENT_LIST_DIR}/msan_suppressions.txt")
    set(ENV{MSAN_OPTIONS} "${_ENV}")
    message(STATUS "
    [Warning] Define MSAN_OPTIONS ENV var:

    export MSAN_OPTIONS=\"${_ENV}\"
    ")
endif()
