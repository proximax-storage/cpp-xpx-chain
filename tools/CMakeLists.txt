cmake_minimum_required(VERSION 3.2)

include_directories(. ${CMAKE_BINARY_DIR}/inc)

add_subdirectory(address)
add_subdirectory(benchmark)
add_subdirectory(health)
add_subdirectory(nemgen)
add_subdirectory(network)
add_subdirectory(statusgen)
add_subdirectory(tools)
add_subdirectory(upgrade)
