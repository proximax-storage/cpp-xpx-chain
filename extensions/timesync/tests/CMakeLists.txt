cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.timesync)

catapult_define_extension_test(timesync api filters handlers test)
target_link_libraries(${TARGET_NAME} tests.catapult.extensions.test)
