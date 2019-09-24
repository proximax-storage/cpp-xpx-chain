### setup gtest
hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)

### setup rocksdb
hunter_add_package(rocksdb)
find_package(RocksDB CONFIG REQUIRED)

### setup benchmark
hunter_add_package(benchmark)
find_package(benchmark CONFIG REQUIRED)
