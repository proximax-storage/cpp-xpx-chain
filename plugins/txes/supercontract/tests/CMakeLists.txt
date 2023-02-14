cmake_minimum_required(VERSION 3.2)

add_subdirectory(test)

set(TARGET_NAME tests.catapult.plugins.supercontract)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/aggregate)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/exchange)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/lock_secret)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/multisig)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/operation)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/service)
catapult_tx_plugin_tests(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} tests.catapult.test.plugins.operation)
