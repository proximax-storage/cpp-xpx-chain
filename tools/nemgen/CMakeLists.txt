cmake_minimum_required(VERSION 3.2)

add_subdirectory(blockhashes)

set(TARGET_NAME catapult.tools.nemgen)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
catapult_executable(${TARGET_NAME})
target_link_libraries(${TARGET_NAME}
	${TARGET_NAME}.blockhashes
	catapult.tools
	catapult.plugins.mosaic
	catapult.plugins.namespace
	catapult.plugins.transfer)
target_link_libraries(${TARGET_NAME} catapult.tools catapult.sdk catapult.plugins.namespace catapult.plugins.transfer catapult.crypto)
catapult_target(${TARGET_NAME})
