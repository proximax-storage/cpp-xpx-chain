# Building from source on Unix

Instructions below are for gcc, but project can be compiled with Clang 9 as well.

NOTE: Commands are using `\` as marker for line continuations

## Prerequisites
 * OpenSSL dev library, at least 1.1.1 (libssl-dev)
 * cmake (at least 3.14)
 * git
 * python 3.x
 * gcc 9.2

## Dependencies
Below, there are listed multiple required dependencies with manual installation. If possible, 
prefer your favourite package manager for simplicity and to avoid unwanted sudos.

### Boost

NOTE: $HOME dir is used below, but any other preferred path can be used instead.
```sh
curl -o boost_1_71_0.tar.gz -SL \
    https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.tar.gz
tar -xzf boost_1_71_0.tar.gz

mkdir boost-build-1.71.0
cd boost_1_71_0
DIR=${HOME} ./bootstrap.sh --prefix=${DIR}/boost-build-1.71.0 &&
./b2 --prefix=${DIR}/boost-build-1.71.0 --without-python -j 4 stage release &&
./b2 --prefix=${DIR}/boost-build-1.71.0 --without-python install
```

### Gtest

```sh
git clone https://github.com/google/googletest
cd googletest
git checkout release-1.8.1

mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON ..
make
sudo make install

cd ../..
rm -rf googletest
```

### Google benchmark

```sh
git clone https://github.com/google/benchmark
cd benchmark
git checkout v1.5.0

mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE_GTEST_TESTS=OFF ..
make
sudo make install

cd ../..
rm -rf benchmark
```

### Mongo

```sh
git clone https://github.com/mongodb/mongo-c-driver
cd mongo-c-driver
git checkout 1.15.1

mkdir _build && cd _build
cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install

cd ../..
rm -rf mongo-c-driver
```

mongocxx
```sh
git clone https://github.com/nemtech/mongo-cxx-driver
cd mongo-cxx-driver
git checkout r3.4.0-nem

mkdir _build && cd _build
cmake -DCMAKE_CXX_STANDARD=17 -DLIBBSON_DIR=/usr/local -DLIBMONGOC_DIR=/usr/local \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install

cd ../..
rm -rf mongo-cxx-driver
```

### ZMQ

libzmq
```sh
git clone git://github.com/zeromq/libzmq
cd libzmq
git checkout v4.3.2

mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install

cd ../..
rm -rf libzmq
```

cppzmq
```sh
git clone https://github.com/zeromq/cppzmq
cd cppzmq
git checkout v4.4.1

mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Release -DCPPZMQ_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install

cd ../..
rm -rf cppzmq
```

### Rocks

Currently ubuntu 18.04 has gflags in version 2.2.1 and snappy in version 1.1.7 which are OK

rocks
```sh
git clone https://github.com/nemtech/rocksdb
cd rocksdb
git checkout v6.6.4-nem

mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Release -DWITH_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install

cd ../..
rm -rf rocksdb
```

### Siriuschain

NOTE: Ensure correct path is passed to DBOOST_ROOT, it should be equal to DIR from Boost intallation. 
```sh
git clone https://github.com/proximax-storage/cpp-xpx-chain
cd cpp-xpx-chain

mkdir _build && cd _build
cmake -DBOOST_ROOT=~/boost-build-1.71.0 -DCMAKE_BUILD_TYPE=Release ..
make publish && make -j $(expr (nproc) - 2)
```
