cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

set(TARGET_NAME tests.catapult.cache_db)

catapult_test_executable_target(${TARGET_NAME} nodeps test)
target_link_libraries(${TARGET_NAME} catapult.io tests.catapult.test.cache)
catapult_add_rocksdb_dependencies(${TARGET_NAME})

set_property(TEST ${TARGET_NAME} PROPERTY LABELS Database_Rocks)
