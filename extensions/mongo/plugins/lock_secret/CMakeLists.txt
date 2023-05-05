cmake_minimum_required(VERSION 3.2)

include_directories(.)
add_subdirectory(src)
if(DO_NOT_SKIP_BUILD_TESTS)
add_subdirectory(tests)
endif()

