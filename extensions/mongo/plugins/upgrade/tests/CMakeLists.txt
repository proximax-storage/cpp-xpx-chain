cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.mongo.plugins.upgrade)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/upgrade)
catapult_mongo_plugin_tests_with_deps(${TARGET_NAME} mappers)

add_subdirectory(int)
add_subdirectory(test)
