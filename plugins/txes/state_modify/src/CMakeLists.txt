cmake_minimum_required(VERSION 3.2)

set(PLUGIN_BASE_NAME catapult.plugins.modifystate)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/metadata)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/config)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/mosaic)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/namespace)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/property)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/upgrade)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/exchange)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/metadata_v2)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/multisig)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/storage)
catapult_tx_plugin_src(${PLUGIN_BASE_NAME})

target_link_libraries(${PLUGIN_BASE_NAME}.deps
        catapult.plugins.config.deps
        catapult.plugins.mosaic.deps
        catapult.plugins.namespace.deps
        catapult.plugins.metadata.deps
        catapult.plugins.property.deps
        catapult.plugins.upgrade.deps
        catapult.plugins.exchange.deps
        catapult.plugins.metadata_v2.deps
        catapult.plugins.storage.deps
        catapult.plugins.multisig.deps
        catapult.plugins.metadata.deps)
