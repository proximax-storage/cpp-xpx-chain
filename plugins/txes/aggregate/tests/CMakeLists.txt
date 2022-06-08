cmake_minimum_required(VERSION 3.2)

add_subdirectory(test)

set(TARGET_NAME tests.catapult.plugins.aggregate)

catapult_tx_plugin_tests(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} tests.catapult.test.local)