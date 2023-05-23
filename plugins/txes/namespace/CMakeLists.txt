cmake_minimum_required(VERSION 3.2)

set(PLUGIN_SDK_FOLDERS model state)
set(PLUGIN_DEPS_FOLDERS cache config handlers observers validators)

include_directories(.)
add_subdirectory(src)

if(DO_NOT_SKIP_BUILD_TESTS)
add_subdirectory(tests)
endif()
