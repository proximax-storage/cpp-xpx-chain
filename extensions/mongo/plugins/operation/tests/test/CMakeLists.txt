cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.test.mongo.plugins.operation)

catapult_add_gtest_dependencies()
catapult_library_target(${TARGET_NAME})
target_link_libraries(${TARGET_NAME}
	tests.catapult.test.cache
	catapult.plugins.operation.deps
	catapult.mongo.plugins.operation.deps
	tests.catapult.test.plugins.operation)
catapult_add_mongo_dependencies(${TARGET_NAME})
