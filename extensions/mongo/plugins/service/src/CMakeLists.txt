cmake_minimum_required(VERSION 3.2)

set(PLUGIN_BASE_NAME catapult.mongo.plugins.service)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/service)
include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/multisig)
catapult_mongo_plugin_src_with_deps(${PLUGIN_BASE_NAME} mappers storages)
target_link_libraries(${PLUGIN_BASE_NAME} catapult.plugins.service.deps catapult.plugins.locksecret.deps)
