cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.sdk)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/multisig)
catapult_test_executable_target(${TARGET_NAME} core builders extensions parsers)
