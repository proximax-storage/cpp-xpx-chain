cmake_minimum_required(VERSION 3.2)

catapult_add_gtest_dependencies()
catapult_library_target(tests.catapult.test.core mocks)

if(DO_NOT_SKIP_BUILD_TESTS)
target_link_libraries(
	tests.catapult.test.core
	tests.catapult.test.nodeps
	${CORE_CATAPULT_LIBS}
	catapult.sdk
	catapult.config_holder)
else()
	target_link_libraries(
			tests.catapult.test.core
			${CORE_CATAPULT_LIBS}
			catapult.sdk
			catapult.config_holder)
endif()