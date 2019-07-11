cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.consumers)

catapult_test_executable_target(${TARGET_NAME} cache test)
target_link_libraries(${TARGET_NAME} tests.catapult.test.local)
