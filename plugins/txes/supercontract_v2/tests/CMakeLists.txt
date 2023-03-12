cmake_minimum_required(VERSION 3.2)

add_subdirectory(test)

set(TARGET_NAME tests.catapult.plugins.supercontract_v2)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/storage)
catapult_tx_plugin_tests(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} catapult.plugins.storage.deps)
