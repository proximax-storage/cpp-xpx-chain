cmake_minimum_required(VERSION 3.2)

catapult_library_target(catapult.dbrb)
target_link_libraries(catapult.dbrb catapult.crypto)
