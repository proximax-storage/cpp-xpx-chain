cmake_minimum_required(VERSION 3.2)

catapult_define_extension_src(storage notification_handlers)
target_link_libraries(catapult.storage catapult.sdk drive torrent-rasterbar Threads::Threads)