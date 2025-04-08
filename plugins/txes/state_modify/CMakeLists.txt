cmake_minimum_required(VERSION 3.2)

include_directories(.)
set(PLUGIN_SDK_FOLDERS model state)
set(PLUGIN_DEPS_FOLDERS config model validators observers utils)


add_subdirectory(src)
if(DO_NOT_SKIP_BUILD_TESTS)
    add_subdirectory(tests)
endif()
