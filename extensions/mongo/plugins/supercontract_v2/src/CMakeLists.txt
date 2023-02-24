cmake_minimum_required(VERSION 3.2)

set(PLUGIN_BASE_NAME catapult.mongo.plugins.supercontract_v2)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/storage)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/supercontract_v2)
catapult_mongo_plugin_src_with_deps(${PLUGIN_BASE_NAME} mappers storages)
target_link_libraries(${PLUGIN_BASE_NAME} catapult.plugins.storage.deps catapult.plugins.supercontract_v2.deps)
