cmake_minimum_required(VERSION 3.2)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/mosaic)
catapult_library_target(tests.catapult.test.local)
target_link_libraries(tests.catapult.test.local
	tests.catapult.test.cache
	tests.catapult.test.net
	catapult.local.server)
