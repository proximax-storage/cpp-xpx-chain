# set version manually to allow build without GFLAGS
# list of versions: https://github.com/ruslo/hunter/blob/master/cmake/projects/rocksdb/hunter.cmake#L27
hunter_config(
    rocksdb
    URL https://github.com/facebook/rocksdb/archive/v5.18.3.tar.gz
    SHA1 dfaaff14d447d9c05f3dbcb84ba6640f4590f634
    CMAKE_ARGS WITH_GFLAGS=0 WITH_TOOLS=0 WITH_TESTS=OFF
)


# set boost to 1.69.0 manually, because 1.71.0 introduces breaking changes
# see https://github.com/boostorg/asio/issues/224
# list of versions https://github.com/ruslo/hunter/blob/master/cmake/projects/Boost/hunter.cmake
hunter_config(
    Boost
    VERSION 1.69.0-p1
)
