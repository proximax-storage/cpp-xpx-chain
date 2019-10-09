cmake_minimum_required(VERSION 3.2)

set(PLUGIN_DEPS_FOLDERS cache config model observers validators plugins state)

include_directories(.)
add_subdirectory(src)
add_subdirectory(tests)
