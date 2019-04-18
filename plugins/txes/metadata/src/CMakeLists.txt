cmake_minimum_required(VERSION 3.2)

set(PLUGIN_BASE_NAME catapult.plugins.metadata)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/mosaic)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/namespace)

catapult_tx_plugin_src(${PLUGIN_BASE_NAME})

# mosaic.deps and namespace.deps are needed because we are using cache in validators
target_link_libraries(${PLUGIN_BASE_NAME}.deps catapult.plugins.mosaic.deps catapult.plugins.namespace.deps)