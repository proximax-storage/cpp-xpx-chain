cmake_minimum_required(VERSION 3.2)

set(PLUGIN_SDK_FOLDERS cache state)
set(PLUGIN_DEPS_FOLDERS config handlers observers validators)

include_directories(.)
add_subdirectory(src)
#add_subdirectory(tests)
