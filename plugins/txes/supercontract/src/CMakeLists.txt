cmake_minimum_required(VERSION 3.2)

set(PLUGIN_BASE_NAME catapult.plugins.supercontract)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/multisig)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/service)
catapult_tx_plugin_src(${PLUGIN_BASE_NAME})
target_link_libraries(${PLUGIN_BASE_NAME}.deps catapult.plugins.multisig.deps catapult.plugins.service.deps)
