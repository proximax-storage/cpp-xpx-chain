cmake_minimum_required(VERSION 3.2)

if(MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /bigobj")
endif()

set(TARGET_NAME tests.catapult.deltaset)

catapult_test_executable_target_header_only(${TARGET_NAME} nodeps test)

target_link_libraries(${TARGET_NAME} catapult.io tests.catapult.test.cache)
catapult_add_rocksdb_dependencies(${TARGET_NAME})

set_property(TEST ${TARGET_NAME} PROPERTY LABELS Database_Rocks)