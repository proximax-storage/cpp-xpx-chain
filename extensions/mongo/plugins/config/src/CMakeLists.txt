cmake_minimum_required(VERSION 3.2)

set(PLUGIN_BASE_NAME catapult.mongo.plugins.config)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/config)
catapult_mongo_plugin_src_with_deps(${PLUGIN_BASE_NAME} mappers storages)
