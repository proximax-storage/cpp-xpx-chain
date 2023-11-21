cmake_minimum_required(VERSION 3.2)

catapult_define_extension(fastfinality)
if(NOT APPLE)
target_link_libraries(extension.fastfinality
    catapult.cache_db
    catapult.cache
    catapult.cache_core
    catapult.cache_tx
    catapult.consumers
    catapult.harvesting_core
    catapult.dbrb
    atomic)
else()
target_link_libraries(extension.fastfinality
        catapult.cache_db
        catapult.cache
        catapult.cache_core
        catapult.cache_tx
        catapult.consumers
        catapult.harvesting_core
        catapult.dbrb)
endif()