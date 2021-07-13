cmake_minimum_required(VERSION 3.2)

catapult_add_gtest_dependencies()
set(TARGET_NAME tests.catapult.test.plugins.service)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/multisig)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/exchange)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/lock_secret)
catapult_library_target(${TARGET_NAME})