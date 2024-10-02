# Building on MacOS Catalina 10.15

Instructions below are for clang, but project compiles with gcc as well.

NOTE: Commands are using `\` as marker for line continuations

### Brew

```sh
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
```

### CMake

```sh
brew install cmake
```

### Boost

```sh
sudo chown -R $(whoami) /usr/local/lib/pkgconfig
brew install boost
```

If there are some issues with boost try to build form source:

NOTE: $HOME dir is used below, but any other preferred path can be used instead.
```sh
wget https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz
tar -xzf boost_1_81_0.tar.gz

mkdir boost-build-1.81.0
cd boost_1_81_0
DIR=${HOME} ./bootstrap.sh --prefix=${DIR}/boost-build-1.81.0 &&
./b2 --prefix=${DIR}/boost-build-1.81.0 --without-python -j 4 stage release &&
./b2 --prefix=${DIR}/boost-build-1.81.0 --without-python install
```

### Gtest

```sh
git clone https://github.com/google/googletest.git googletest.git
cd googletest.git
git checkout release-1.12.0

mkdir _build && cd _build
cmake -DCMAKE_CXX_COMPILER="clang++" -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON ..
make
sudo make install
cd ../..
sudo rm -R googletest.git
```

If there are problems after installation, the gtest could be installed from brew:

```sh
brew install googletest
```

### Google benchmark

```sh
git clone https://github.com/google/benchmark.git google.benchmark.git
cd google.benchmark.git
git checkout v1.8.3

mkdir _build && cd _build
cmake -DCMAKE_CXX_COMPILER="clang++" -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE_GTEST_TESTS=OFF ..
make
sudo make install
cd ../..
sudo rm -R google.benchmark.git
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
cd ../..
sudo rm -R mongo-c-driver.git
```

mongocxx

```sh
git clone https://github.com/mongodb/mongo-cxx-driver.git
cd mongo-cxx-driver.git
git checkout r3.7.0

mkdir _build && cd _build
cmake  -DPYTHON_EXECUTABLE=$(which python3) -DCMAKE_CXX_STANDARD=17 -DLIBBSON_DIR=/usr/local -DLIBMONGOC_DIR=/usr/local \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
make -j 10
sudo make install
cd ../..
sudo rm -R mongo-cxx-driver.git
```

### ZMQ

libzmq (v4.3.2 or higher)
cppzmq (v4.4.1 or higher)

```sh
brew install zeromq
brew install cppzmq
```

### Rocks

Working version is v8.5.3. But the newest could OK too. If there are issues with newer version return to the v8.5.3.

Use homebrew
```sh
brew install rocksdb
```

Or install from source

```sh
git clone https://github.com/facebook/rocksdb.git
cd rocksdb
git checkout v8.5.3

mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Release -DWITH_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local -DWITH_SNAPPY=1 ..
make
sudo make install
cd ../..
sudo rm -R rocksdb
```

### Openssl

Most probably the default installed openssl is not suitable. The openssl@1.1 should be installed:

```shell
brew install openssl@1.1
```

If there are still issues:

1. Link installed openssl@1.1
    ```sh
    ln -s /opt/homebrew/Cellar/openssl@1.1/1.1.1w/bin/openssl /usr/local/bin/
    ln -s /opt/homebrew/Cellar/openssl@1.1/1.1.1w/include/openssl /usr/local/include/openssl
    ```
2. Update your `~/.bash_profile`
    ```text
    export LIBRARY_PATH="$LIBRARY_PATH:/usr/local/lib"
    ```

### Blockchain

**_NOTE_**: After first installation of libraries above would be good to restart your Mac or log out and log in again.

Use `-DGTEST_INCLUDE_DIR=/usr/local/include` if gtest not found automatically.
Use `-DBOOST_ROOT=~/boost-build-1.81.0` if BOOST was built from source. Example:
```sh
cmake -DBOOST_ROOT=~/boost-build-1.81.0 -DDO_NOT_SKIP_BUILD_TESTS=TRUE -DGTEST_INCLUDE_DIR=/usr/local/include -DCMAKE_BUILD_TYPE=Release
```

```sh
git clone https://github.com/proximax-storage/cpp-xpx-chain.git
cd cpp-xpx-chain
git submodule update --init --remote --recursive

mkdir _build && cd _build
cmake -DDO_NOT_SKIP_BUILD_TESTS=TRUE -DGTEST_INCLUDE_DIR=/usr/local/include -DCMAKE_BUILD_TYPE=Release
make publish && make -j 4
```
