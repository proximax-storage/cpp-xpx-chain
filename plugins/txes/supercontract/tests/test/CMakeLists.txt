cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.test.plugins.supercontract)

catapult_add_gtest_dependencies()
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/operation)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/service)
catapult_library_target(${TARGET_NAME})
#target_link_libraries(${TARGET_NAME} tests.catapult.test.plugins.service)
