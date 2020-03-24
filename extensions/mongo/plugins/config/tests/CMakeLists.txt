cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.mongo.plugins.config)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/config)
catapult_mongo_plugin_tests_with_deps(${TARGET_NAME} mappers)
target_link_libraries(${TARGET_NAME} tests.catapult.test.plugins.config)

add_subdirectory(int)
add_subdirectory(test)
