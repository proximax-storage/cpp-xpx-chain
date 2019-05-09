cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.plugins.metadata)

add_subdirectory(test)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/mosaic)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/namespace)

catapult_tx_plugin_tests(${TARGET_NAME})
