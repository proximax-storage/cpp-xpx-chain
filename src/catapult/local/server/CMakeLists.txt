cmake_minimum_required(VERSION 3.2)

catapult_library_target(catapult.local.server)
target_link_libraries(catapult.local.server catapult.local catapult.handlers   catapult.plugins.config)
