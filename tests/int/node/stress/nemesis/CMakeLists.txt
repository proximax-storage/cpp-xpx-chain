cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME tests.catapult.int.node.stress.nemesis)

catapult_library(${TARGET_NAME})
target_link_libraries(${TARGET_NAME}
	catapult.tools.nemgen.blockhashes
	catapult.plugins.config
	catapult.plugins.mosaic
	catapult.plugins.namespace
	catapult.plugins.transfer
	catapult.plugins.upgrade
	catapult.plugins.committee
	catapult.tools
	catapult.sdk
	catapult.crypto
	catapult.config
	catapult.config_holder)
catapult_target(${TARGET_NAME})

# test has chain dependency so it must be able to access src
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/mosaic)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/namespace)
#include_directories(. ${CMAKE_BINARY_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/tools/nemgen/blockhashes)
