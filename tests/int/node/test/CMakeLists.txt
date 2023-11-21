cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.int.node.test)

catapult_add_gtest_dependencies()
catapult_library_target(${TARGET_NAME})
target_link_libraries(${TARGET_NAME}
    tests.catapult.test.local
    catapult.plugins.config
    catapult.plugins.upgrade
    catapult.plugins.committee
    catapult.plugins.dbrb
    catapult.plugins.metadata_v2)
