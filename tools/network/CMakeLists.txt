cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME catapult.tools.network)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
catapult_executable(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} catapult.tools catapult.nodediscovery)
catapult_target(${TARGET_NAME})
