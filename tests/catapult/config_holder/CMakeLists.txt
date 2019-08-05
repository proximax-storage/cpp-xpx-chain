cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.config_holder)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

catapult_test_executable_target(${TARGET_NAME} core)
target_link_libraries(${TARGET_NAME} catapult.plugins.config.sdk tests.catapult.test.local)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/config)
