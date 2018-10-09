cmake_minimum_required(VERSION 3.2)

catapult_library_target(tests.catapult.extensions.test test)
catapult_test_executable_target(tests.catapult.extensions local test)
