cmake_minimum_required(VERSION 3.2)

catapult_library_target(catapult.crypto)
target_link_libraries(catapult.crypto catapult.utils external)
target_include_directories(catapult.crypto PUBLIC ../../../external)
