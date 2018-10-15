cmake_minimum_required(VERSION 3.2)

catapult_test_executable_target(tests.catapult.consumers cache test)
target_link_libraries(tests.catapult.consumers tests.catapult.extensions.test catapult.config)