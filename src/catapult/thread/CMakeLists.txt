cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
catapult_library_target(catapult.thread)
target_link_libraries(catapult.thread catapult.ionet catapult.model catapult.utils)
