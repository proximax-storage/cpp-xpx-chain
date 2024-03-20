cmake_minimum_required(VERSION 3.2)

catapult_define_extension_test(fastfinality mocks)
if(NOT APPLE)
target_link_libraries(tests.catapult.fastfinality
    catapult.cache_db
    catapult.cache
    catapult.cache_core
    catapult.cache_tx
    catapult.consumers
    catapult.harvesting_core
    catapult.dbrb
    atomic)
else()
    target_link_libraries(tests.catapult.fastfinality
            catapult.cache_db
            catapult.cache
            catapult.cache_core
            catapult.cache_tx
            catapult.consumers
            catapult.harvesting_core
            catapult.dbrb)
endif()