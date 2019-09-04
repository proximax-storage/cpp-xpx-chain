# sanitizers should be executed with gcc-7
if(SANITIZE_ADDRESS OR SANITIZE_THREAD OR SANITIZE_UNDEFINED)
    find_program(CMAKE_C_COMPILER gcc-7)
    find_program(CMAKE_CXX_COMPILER g++-7)

    if(NOT CMAKE_C_COMPILER)
        message(FATAL_ERROR "gcc-7 not found")
    endif()

    if(NOT CMAKE_CXX_COMPILER)
        message(FATAL_ERROR "g++-7 not found")
    endif()

    set(
        CMAKE_C_COMPILER
        "${CMAKE_C_COMPILER}"
        CACHE
        STRING
        "C compiler"
        FORCE
    )

    set(
        CMAKE_CXX_COMPILER
        "${CMAKE_CXX_COMPILER}"
        CACHE
        STRING
        "C++ compiler"
        FORCE
    )
endif()

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

    set(ENV{ASAN_OPTIONS} verbosity=1:debug=1:detect_leaks=1:check_initialization_order=1:alloc_dealloc_mismatch=true:use_odr_indicator=true)
elseif(SANITIZE_THREAD)
    message(STATUS "SANITIZE_THREAD enabled")
    add_cache_flag(CMAKE_CXX_FLAGS "-fsanitize=thread")
    add_cache_flag(CMAKE_CXX_FLAGS "-g")

    add_cache_flag(CMAKE_C_FLAGS "-fsanitize=thread")
    add_cache_flag(CMAKE_C_FLAGS "-g")

    add_cache_flag(CMAKE_EXE_LINKER_FLAGS "-fsanitize=thread")
    add_cache_flag(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=thread")

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
endif()
