cmake_minimum_required(VERSION 3.2)

catapult_test_executable_target(tests.catapult.extensions local)
target_link_libraries(tests.catapult.extensions tests.catapult.test.nemesis catapult.plugins.config)
