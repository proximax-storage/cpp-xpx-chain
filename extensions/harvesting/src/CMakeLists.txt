cmake_minimum_required(VERSION 3.2)

catapult_define_extension_src(harvesting)
target_link_libraries(catapult.harvesting
    catapult.harvesting_core
    catapult.model
    catapult.cache
    catapult.chain
    catapult.dbrb)
