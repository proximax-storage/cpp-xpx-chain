cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
catapult_library_target(catapult.tools)
target_link_libraries(catapult.tools ${CORE_CATAPULT_LIBS} catapult.api catapult.config catapult.net catapult.version catapult.crypto)
target_link_libraries(catapult.tools ${CORE_CATAPULT_LIBS} catapult.api catapult.config catapult.net catapult.sdk catapult.version)
