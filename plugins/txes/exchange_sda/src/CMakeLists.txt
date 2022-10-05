cmake_minimum_required(VERSION 3.2)

set(PLUGIN_BASE_NAME catapult.plugins.exchangesda)
catapult_tx_plugin_src(${PLUGIN_BASE_NAME})

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/mosaic)
# mosaic.deps is needed because the mosaic cache is needed
target_link_libraries(${PLUGIN_BASE_NAME}.deps catapult.plugins.mosaic.deps)
