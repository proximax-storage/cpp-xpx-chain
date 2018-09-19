cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.harvesting)

catapult_define_extension_test(harvesting)
target_link_libraries(${TARGET_NAME} tests.catapult.extensions.test)