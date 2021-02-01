cmake_minimum_required(VERSION 3.2)

catapult_add_gtest_dependencies()
add_subdirectory(test)

set(TARGET_NAME tests.catapult.plugins.exchange)

catapult_tx_plugin_tests(${TARGET_NAME})

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/mosaic)
