cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME spammer)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

include_directories(. ${CMAKE_BINARY_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src)

catapult_executable(${TARGET_NAME})
if(DO_NOT_SKIP_BUILD_TESTS)
    target_link_libraries(${TARGET_NAME} catapult.sdk catapult.net tests.catapult.test.nodeps tests.catapult.test.core)
else()
    target_link_libraries(${TARGET_NAME} catapult.sdk catapult.net)
endif()
catapult_target(${TARGET_NAME})
