cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.test.plugins.exchange)

catapult_library_target(${TARGET_NAME})

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/mosaic)