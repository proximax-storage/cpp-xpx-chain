cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.model)

catapult_test_executable_target(${TARGET_NAME} core)
target_link_libraries(${TARGET_NAME} external tests.catapult.test.local)
