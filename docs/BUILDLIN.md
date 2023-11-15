# Building on Ubuntu 22.04 (LTS)

Use the script below:

```
set -e
set -x

cd /tmp

apt-get update -y && apt-get upgrade -y && apt-get clean && \
  DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends \
  cmake \
  git \
  curl \
  wget \
  build-essential \
  gcc-12 \
  software-properties-common \
  pkg-config \
  libssl-dev \
  libsasl2-dev \
  libtool \
  libboost-dev \
  libboost-atomic-dev \
  libboost-date-time-dev \
  libboost-regex-dev \
  libboost-system-dev \
  libboost-timer-dev \
  libboost-chrono-dev \
  libboost-log-dev \
  libboost-thread-dev \
  libboost-filesystem-dev \
  libboost-program-options-dev \
  libboost-stacktrace-dev \
  libboost-random-dev \
  libgtest-dev \
  libbenchmark-dev \
  librocksdb-dev \
  libzmq3-dev \
  libmongoc-1.0-0 \
  libmongoc-dev \
  && apt-get clean && rm -rf /var/lib/apt/lists/*

# Replace gcc with gcc-12
rm /usr/bin/gcc && ln -s /usr/bin/gcc-12 /usr/bin/gcc

cd /usr/local/src

curl -OL https://github.com/zeromq/cppzmq/archive/v4.9.0.tar.gz && \
  tar xzf v4.9.0.tar.gz && \
  cd cppzmq-4.9.0 && \
  mkdir build && \
  cd build && \
  cmake -DCPPZMQ_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .. && \
  make -j4 && \
  make install

# Install mongo-cxx
curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.7.0/mongo-cxx-driver-r3.7.0.tar.gz  && \
tar -xzf mongo-cxx-driver-r3.7.0.tar.gz  && \
cd mongo-cxx-driver-r3.7.0/build && \
  cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DBSONCXX_POLY_USE_BOOST=1 .. && \
  make -j4 && \
  make install

# Clean Downloads
rm v4.9.0.tar.gz mongo-cxx-driver-r3.7.0.tar.gz 
```
# Building on Ubuntu 18.04 (LTS)

Instructions below are for gcc, but project compiles with clang 9 as well.

NOTE: Commands are using `\` as marker for line continuations

## Prerequisites

 * OpenSSL dev library, at least 1.1.1 (libssl-dev)
 * cmake (at least 3.14)
 * git
 * python 3.x
 * gcc 9.2

### Boost

```sh
curl -o boost_1_71_0.tar.gz -SL \
    https://boostorg.jfrog.io/artifactory/main/release/1.71.0/source/boost_1_71_0.tar.gz
tar -xzf boost_1_71_0.tar.gz

## WARNING: below use $HOME rather than ~ - boost scripts might treat it literally
mkdir boost-build-1.71.0
cd boost_1_71_0
./bootstrap.sh --prefix=${HOME}/boost-build-1.71.0
./b2 --prefix=${HOME}/boost-build-1.71.0 --without-python -j 4 stage release
./b2 --prefix=${HOME}/boost-build-1.71.0 --without-python install
```

### Gtest

```sh
git clone https://github.com/google/googletest.git googletest.git
cd googletest.git
git checkout release-1.8.1

mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON ..
make
sudo make install
```

### Google benchmark

```sh
git clone https://github.com/google/benchmark.git google.benchmark.git
cd google.benchmark.git
git checkout v1.5.0

mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE_GTEST_TESTS=OFF ..
make
sudo make install
```

### Mongo

mongo-c

```sh
git clone https://github.com/mongodb/mongo-c-driver.git mongo-c-driver.git
cd mongo-c-driver.git
git checkout 1.20

mkdir _build && cd _build
cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install
```

mongocxx
```sh
git clone https://github.com/mongodb/mongo-cxx-driver.git
cd mongo-cxx-driver.git
git checkout r3.6.0

mkdir _build && cd _build
cmake -DCMAKE_CXX_STANDARD=17 -DLIBBSON_DIR=/usr/local -DLIBMONGOC_DIR=/usr/local \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install
```

### ZMQ

libzmq
```sh
git clone git://github.com/zeromq/libzmq.git libzmq.git
cd libzmq.git
git checkout v4.3.2

mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install
```

cppzmq
```sh
git clone https://github.com/zeromq/cppzmq.git cppzmq.git
cd cppzmq.git
git checkout v4.4.1

mkdir _build && cd _build
cmake -DCPPZMQ_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release -DCPPZMQ_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install
```

### Rocks

Currently ubuntu 18.04 has gflags in version 2.2.1 and snappy in version 1.1.7 which are OK

rocks
```sh
git clone https://github.com/facebook/rocksdb.git
cd rocksdb.git
git checkout v6.6.4

mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Release -DWITH_TESTS=OFF -DWITH_BENCHMARK_TOOLS=OFF -DWITH_TOOLS=OFF -DFAIL_ON_WARNINGS=OFF -DWITH_RUNTIME_DEBUG=OFF -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install
```

### Blockchain

```sh
git clone https://github.com/proximax-storage/cpp-xpx-chain.git
cd cpp-xpx-chain

mkdir _build && cd _build
cmake -DBOOST_ROOT=~/boost-build-1.71.0 -DCMAKE_BUILD_TYPE=Release .. -B .
make publish && make -j 4
```
