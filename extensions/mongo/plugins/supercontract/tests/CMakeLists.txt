cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.mongo.plugins.supercontract)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/operation)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/service)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/supercontract)
catapult_mongo_plugin_tests_with_deps(${TARGET_NAME} mappers)
target_link_libraries(${TARGET_NAME}
	tests.catapult.test.cache
	catapult.plugins.supercontract.deps
	catapult.mongo.plugins.supercontract.deps
	tests.catapult.test.plugins.supercontract)

add_subdirectory(int)
add_subdirectory(test)
