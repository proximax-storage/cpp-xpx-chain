### setup boost
hunter_add_package(Boost COMPONENTS atomic system date_time regex timer chrono log thread filesystem program_options random)
find_package(Boost CONFIG COMPONENTS atomic system date_time regex timer chrono log thread filesystem program_options random REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

### setup gtest
hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)
find_package(GMock CONFIG REQUIRED)

### setup rocksdb
hunter_add_package(rocksdb)
find_package(RocksDB CONFIG REQUIRED)

### setup benchmark
hunter_add_package(benchmark)
find_package(benchmark CONFIG REQUIRED)
