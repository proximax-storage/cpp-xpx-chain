cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME catapult.broker)

catapult_executable(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} catapult.local.broker catapult.process)
catapult_target(${TARGET_NAME})
