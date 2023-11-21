cmake_minimum_required(VERSION 3.2)

catapult_add_gtest_dependencies()
catapult_header_only_target(tests.catapult.plugins.lockshared cache observers state test validators)
