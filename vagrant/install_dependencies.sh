#!/usr/bin/env bash

sed -i.bak -e "s%http://[^ ]\+%http://linux.yz.yamagata-u.ac.jp/ubuntu/%g" /etc/apt/sources.list

apt-get update -y && apt-get upgrade -y && apt-get clean && apt-get install -y --no-install-recommends \
    git \
    curl \
    wget \
    vim \
    autoconf \
    automake \
    apt-file \
    build-essential \
    software-properties-common \
    pkg-config \
    python3 \
    libc6-dev \
    libssl-dev \
    libsasl2-dev \
    libtool \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# gcc,g++ 7
add-apt-repository ppa:ubuntu-toolchain-r/test \
    && apt-get update && apt-get install -y --no-install-recommends gcc-7 g++-7 \
    && apt-get clean && rm -rf /var/lib/apt/lists/* \
    && rm /usr/bin/gcc /usr/bin/g++ \
    && ln -s /usr/bin/gcc-7 /usr/bin/gcc \
    && ln -s /usr/bin/g++-7 /usr/bin/g++

# cmake
git clone https://gitlab.kitware.com/cmake/cmake.git -b v3.11.1 --depth 1 \
    && cd cmake \
    && ./bootstrap --prefix=/usr/local && make -j4 && make install \
    && cd - \
    && rm -R cmake

# boost
wget https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.gz \
    && tar xzf boost_1_65_1.tar.gz && cd boost_1_65_1 \
    && ./bootstrap.sh && ./b2 toolset=gcc install --prefix=/usr/local -j4 \
    && cd - \
    && rm -R boost_1_65_1.tar.gz \
    && rm -R boost_1_65_1

# gtest
git clone https://github.com/google/googletest.git -b release-1.8.0 --depth 1 \
    && mkdir -p googletest/_build && cd googletest/_build \
    && cmake -DCMAKE_INSTALL_PREFIX=/usr/local .. && make -j4 && make install \
    && cd - \
    && rm -R googletest

# rocksdb
git clone https://github.com/facebook/rocksdb.git -b v5.12.4 --depth 1 \
    && mkdir -p rocksdb/_build && cd rocksdb/_build \
    && cmake -DCMAKE_INSTALL_PREFIX=/usr/local .. && make -j4 && make install \
    && cd - \
    && rm -R rocksdb

# zmqlib
git clone https://github.com/zeromq/libzmq.git -b v4.2.3 --depth 1 \
    && mkdir -p libzmq/_build && cd libzmq/_build \
    && cmake -DCMAKE_INSTALL_PREFIX=/usr/local .. && make -j4 && make install \
    && cd - \
    && rm -R libzmq

# cppzmq
git clone https://github.com/zeromq/cppzmq.git -b v4.2.3 --depth 1 \
    && mkdir -p cppzmq/_build && cd cppzmq/_build \
    && cmake -DCMAKE_INSTALL_PREFIX=/usr/local .. && make -j4 && make install \
    && cd - \
    && rm -R cppzmq

# mongo-c
git clone https://github.com/mongodb/mongo-c-driver.git -b 1.4.3 --depth 1 && cd mongo-c-driver \
    && ./autogen.sh && ./configure --disable-automatic-init-and-cleanup --prefix=/usr/local \
    && make -j4 && make install \
    && cd - \
    && rm -R mongo-c-driver

# mongo-cxx
git clone https://github.com/mongodb/mongo-cxx-driver.git -b r3.0.2 --depth 1 && cd mongo-cxx-driver/build \
    && cmake -DBSONCXX_POLY_USE_BOOST=1 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .. \
    && make -j4 && make install

