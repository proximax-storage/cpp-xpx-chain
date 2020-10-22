cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.mongo.plugins.exchange)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/exchange)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/mosaic)

catapult_mongo_plugin_tests_with_deps(${TARGET_NAME} mappers)
target_link_libraries(${TARGET_NAME} tests.catapult.test.plugins.exchange)

add_subdirectory(int)
add_subdirectory(test)
