cmake_minimum_required(VERSION 3.2)

include_directories(
    ${PROJECT_SOURCE_DIR}/cpp-xpx-storage-sdk/include
    ${PROJECT_SOURCE_DIR}/cpp-xpx-storage-sdk/cereal/include
)

include_directories(. ${CMAKE_BINARY_DIR}/inc)

catapult_define_extension(storage)
