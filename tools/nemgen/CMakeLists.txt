cmake_minimum_required(VERSION 3.2)
include_directories(. ${CMAKE_BINARY_DIR})
add_subdirectory(blockhashes)

set(TARGET_NAME catapult.tools.nemgen)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
catapult_executable(${TARGET_NAME})
target_link_libraries(${TARGET_NAME}
	${TARGET_NAME}.blockhashes
	catapult.plugins.config
	catapult.tools
	catapult.sdk
	catapult.plugins.exchange
	catapult.plugins.mosaic
	catapult.plugins.namespace
	catapult.plugins.transfer
	catapult.plugins.upgrade
	catapult.plugins.committee
	catapult.plugins.metadata
	catapult.plugins.multisig
	catapult.plugins.metadata_v2
	catapult.extensions
	catapult.plugins.storage
	catapult.plugins.property
	catapult.crypto
	catapult.config
	catapult.cache
	catapult.config_holder)
catapult_target(${TARGET_NAME})

# tool has chain dependency so it must be able to access these dirs.
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/sdk)
# tool has chain dependency so it must be able to access these dirs.