cmake_minimum_required(VERSION 3.2)

set(PLUGIN_BASE_NAME catapult.mongo.plugins.reputation)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/reputation)
catapult_mongo_plugin_src_with_deps(${PLUGIN_BASE_NAME} mappers storages)
