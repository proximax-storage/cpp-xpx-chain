cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME catapult.sdk)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/multisig)
catapult_library_target(${TARGET_NAME} builders extensions parsers)
target_link_libraries(${TARGET_NAME} catapult.model
	catapult.plugins.aggregate
	catapult.plugins.mosaic.sdk
	catapult.plugins.namespace.sdk
	catapult.plugins.property.sdk)

find_program(PYTHON_EXECUTABLE python3 python)

add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/catapult/catapult.h
	DEPENDS ${PROJECT_SOURCE_DIR}/scripts/sdk/publishSdk.py
	COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/sdk/publishSdk.py ARGS --r ${PROJECT_SOURCE_DIR} --p ${CMAKE_BINARY_DIR}/inc)

add_custom_target(
	publish
	DEPENDS ${CMAKE_BINARY_DIR}/catapult/catapult.h)
