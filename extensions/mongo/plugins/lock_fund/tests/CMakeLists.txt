cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.mongo.plugins.lockfund)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/lock_fund)
catapult_mongo_plugin_tests_with_deps_no_test(${TARGET_NAME} mappers)

