cmake_minimum_required(VERSION 3.2)

include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src)

set(PLUGIN_CATAPULT_LIBS
	${CORE_CATAPULT_LIBS}
	catapult.cache
	catapult.cache_core
	catapult.chain
	catapult.config
	catapult.config_holder
	catapult.observers
	catapult.plugins
	catapult.state
	catapult.validators)

add_subdirectory(coresystem)
add_subdirectory(services)
add_subdirectory(txes)
