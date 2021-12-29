cmake_minimum_required(VERSION 3.2)

catapult_define_extension_test(storage)
target_link_libraries(tests.catapult.storage crypto drive)
