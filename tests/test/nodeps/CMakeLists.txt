cmake_minimum_required(VERSION 3.2)

catapult_add_gtest_dependencies()
catapult_library_target(tests.catapult.test.nodeps)
target_link_libraries(tests.catapult.test.nodeps catapult.utils catapult.version ${CMAKE_DL_LIBS})
