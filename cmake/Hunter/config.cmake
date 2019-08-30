# set version manually to allow build without GFLAGS
# list of versions: https://github.com/ruslo/hunter/blob/master/cmake/projects/rocksdb/hunter.cmake#L27
hunter_config(
    rocksdb
    VERSION 5.14.2
    CMAKE_ARGS WITH_GFLAGS=0 WITH_TOOLS=0 CMAKE_POSITION_INDEPENDENT_CODE=ON
)


# set boost to 1.69.0 manually, because 1.71.0 introduces breaking changes
# see https://github.com/boostorg/asio/issues/224
# list of versions https://github.com/ruslo/hunter/blob/master/cmake/projects/Boost/hunter.cmake
hunter_config(
    Boost
    VERSION 1.69.0-p1
    CMAKE_ARGS CMAKE_POSITION_INDEPENDENT_CODE=ON
)
