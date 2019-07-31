cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.chain)

catapult_test_executable_target(${TARGET_NAME} cache test)
target_link_libraries(${TARGET_NAME} catapult.config tests.catapult.test.local)
