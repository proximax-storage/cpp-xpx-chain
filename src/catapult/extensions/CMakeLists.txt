cmake_minimum_required(VERSION 3.2)

catapult_library_target(catapult.extensions)
target_link_libraries(catapult.extensions catapult.chain catapult.plugins catapult.subscribers catapult.config_holder catapult.plugins.config)
