cmake_minimum_required(VERSION 3.2)

add_subdirectory(nemesis)

set(TARGET_NAME tests.catapult.int.node.stress)

catapult_int_test_executable_target(${TARGET_NAME} test)
target_link_libraries(${TARGET_NAME} tests.catapult.int.node.test tests.catapult.test.nemesis catapult.local.server tests.catapult.int.node.stress.nemesis)

set_property(TEST ${TARGET_NAME} PROPERTY LABELS Stress)

# tool has chain dependency so it must be able to access these dirs.
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/mosaic)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/namespace)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes)
include_directories(${PROJECT_SOURCE_DIR}/src)
