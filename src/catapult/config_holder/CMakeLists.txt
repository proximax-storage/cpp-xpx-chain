cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME catapult.config_holder)

catapult_library_target(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} catapult.config)
