cmake_minimum_required(VERSION 3.2)

set(PLUGIN_SDK_FOLDERS model)
set(PLUGIN_DEPS_FOLDERS config validators observers)

include_directories(.)
add_subdirectory(src)
add_subdirectory(tests)
