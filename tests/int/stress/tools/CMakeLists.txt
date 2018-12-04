cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.int.stress.patriciaTreeGenerator)
catapult_executable(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} catapult.tools gtest)
catapult_target(${TARGET_NAME})
