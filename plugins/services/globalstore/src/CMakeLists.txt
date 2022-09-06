cmake_minimum_required(VERSION 3.2)

set(PLUGIN_BASE_NAME catapult.plugins.globalstore)

# create a sdk lib
catapult_library_target(${PLUGIN_BASE_NAME}.sdk state)
target_link_libraries(${PLUGIN_BASE_NAME}.sdk catapult.model)

# create a cache lib
catapult_library_target(${PLUGIN_BASE_NAME}.cache cache)
target_link_libraries(${PLUGIN_BASE_NAME}.cache catapult.cache catapult.state)

# create a deps lib
catapult_library_target(${PLUGIN_BASE_NAME}.deps config cache state)
target_link_libraries(${PLUGIN_BASE_NAME}.deps ${PLUGIN_CATAPULT_LIBS} ${PLUGIN_BASE_NAME}.cache)

# create a plugin dll
catapult_shared_library_target(${PLUGIN_BASE_NAME} plugins)
target_link_libraries(${PLUGIN_BASE_NAME} ${PLUGIN_BASE_NAME}.deps  compile_time_hash)
