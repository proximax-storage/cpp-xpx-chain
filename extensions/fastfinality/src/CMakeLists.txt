cmake_minimum_required(VERSION 3.2)

catapult_define_extension_src(fastfinality utils dbrb weighted_voting)
target_link_libraries(catapult.fastfinality catapult.sdk)
