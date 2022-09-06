cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.plugins.globalstore)

add_subdirectory(test)

catapult_test_executable_target(${TARGET_NAME} cache cache plugins config state)
target_link_libraries(${TARGET_NAME} tests.catapult.test.plugins.globalstore)