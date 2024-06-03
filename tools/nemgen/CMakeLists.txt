cmake_minimum_required(VERSION 3.2)

add_subdirectory(blockhashes)

set(TARGET_NAME catapult.tools.nemgen)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
catapult_executable(${TARGET_NAME})
target_link_libraries(${TARGET_NAME}
	${TARGET_NAME}.blockhashes
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

# tool has chain dependency so it must be able to access these dirs.
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes)
include_directories(${PROJECT_SOURCE_DIR}/src)
