cmake_minimum_required(VERSION 3.2)

set(PLUGIN_BASE_NAME catapult.mongo.plugins.exchange)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/exchange)
catapult_mongo_plugin_src_with_deps(${PLUGIN_BASE_NAME} mappers storages)

# exchange.sdk is needed because the mongo exchange cache storage requires the exchange entry
target_link_libraries(${PLUGIN_BASE_NAME}.deps catapult.plugins.exchange.sdk)
