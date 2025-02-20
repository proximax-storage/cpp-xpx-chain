cmake_minimum_required(VERSION 3.2)

set(PLUGIN_BASE_NAME catapult.mongo.plugins.exchangesda)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/exchange_sda)
catapult_mongo_plugin_src_with_deps(${PLUGIN_BASE_NAME} mappers storages)

# exchangesda.sdk is needed because the mongo exchangesda cache storage requires the exchangesda entry
target_link_libraries(${PLUGIN_BASE_NAME}.deps catapult.plugins.exchangesda.sdk)
