cmake_minimum_required(VERSION 3.2)

set(PLUGIN_SDK_FOLDERS model state)
set(PLUGIN_DEPS_FOLDERS config cache handlers observers validators)

include_directories(.)
add_subdirectory(src)
# DO NOT COMPILE TESTS AS PLUGIN IS OBSOLETE AND NO LONGER SUPPORTED
#add_subdirectory(tests)
