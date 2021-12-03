cmake_minimum_required(VERSION 3.2)

catapult_define_extension_src(storage notification_handlers utils)
target_link_libraries(catapult.storage catapult.sdk drive2 torrent-rasterbar Threads::Threads)

catapult_shared_library_target(drive2)
