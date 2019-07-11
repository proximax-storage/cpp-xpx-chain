cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME catapult.server)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
catapult_executable(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} catapult.local catapult.version catapult.config_holder)
catapult_target(${TARGET_NAME})
