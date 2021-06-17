cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME catapult.tools.health)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
catapult_executable(${TARGET_NAME})
add_subdirectory(prometheus-cpp)
target_link_libraries(${TARGET_NAME} catapult.tools)
catapult_target(${TARGET_NAME})
