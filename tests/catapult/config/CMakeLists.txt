cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

catapult_test_executable_target(tests.catapult.config core)
