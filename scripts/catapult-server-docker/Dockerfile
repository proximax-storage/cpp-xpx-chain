FROM ubuntu:16.04

WORKDIR /tmp

RUN sed -i.bak -e "s%http://[^ ]\+%http://linux.yz.yamagata-u.ac.jp/ubuntu/%g" /etc/apt/sources.list
RUN apt-get update -y && apt-get upgrade -y && apt-get clean && apt-get install -y --no-install-recommends \
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
RUN add-apt-repository ppa:ubuntu-toolchain-r/test \
  && apt-get update && apt-get install -y --no-install-recommends gcc-7 g++-7 \
  && apt-get clean && rm -rf /var/lib/apt/lists/* \
  && rm /usr/bin/gcc /usr/bin/g++ \
  && ln -s /usr/bin/gcc-7 /usr/bin/gcc \
  && ln -s /usr/bin/g++-7 /usr/bin/g++

# cmake
RUN git clone https://gitlab.kitware.com/cmake/cmake.git -b v3.11.1 --depth 1 \
  && cd cmake \
  && ./bootstrap --prefix=/usr/local && make -j4 && make install \
  && cd -

# boost
RUN wget https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.gz \
  && tar xzf boost_1_69_0.tar.gz && cd boost_1_69_0 \
  && ./bootstrap.sh && ./b2 toolset=gcc install --prefix=/usr/local -j4 \
  && cd - && rm boost_1_69_0.tar.gz && rm -rf boost_1_69_0.tar.gz

# gtest
RUN git clone https://github.com/google/googletest.git -b release-1.8.0 --depth 1 \
  && mkdir -p googletest/_build && cd googletest/_build \
  && cmake -DCMAKE_INSTALL_PREFIX=/usr/local .. && make -j4 && make install \
  && cd -

# benchmark
RUN git clone https://github.com/google/benchmark.git google.benchmark.git -b v1.4.1 --depth 1 \
  && cd google.benchmark.git && mkdir _build && cd _build \
  && cmake -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE_GTEST_TESTS=OFF .. && make -j4 && make install \
  && cd - && rm -rf google.benchmark.git

# rocksdb
RUN git clone https://github.com/facebook/rocksdb.git -b 5.18.fb --depth 1 \
  && mkdir -p rocksdb/_build && cd rocksdb/_build \
  && cmake -DCMAKE_INSTALL_PREFIX=/usr/local .. && make -j4 && make install \
  && cd -

# zmqlib
RUN git clone https://github.com/zeromq/libzmq.git -b v4.2.3 --depth 1 \
  && mkdir -p libzmq/_build && cd libzmq/_build \
  && cmake -DCMAKE_INSTALL_PREFIX=/usr/local .. && make -j4 && make install \
  && cd -

# cppzmq
RUN git clone https://github.com/zeromq/cppzmq.git -b v4.2.3 --depth 1 \
  && mkdir -p cppzmq/_build && cd cppzmq/_build \
  && cmake -DCMAKE_INSTALL_PREFIX=/usr/local .. && make -j4 && make install \
  && cd -

# mongo-c
RUN git clone https://github.com/mongodb/mongo-c-driver.git mongo-c-driver.git -b 1.13.0 --depth 1 && cd mongo-c-driver.git \
  && mkdir _build && cd _build \
  && cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .. \
  && make -j4 && make install \
  && cd - && rm -rf mongo-c-driver.git

# mongo-cxx
RUN git clone https://github.com/mongodb/mongo-cxx-driver.git mongo-cxx-driver.git -b r3.4.0 --depth 1 && cd mongo-cxx-driver.git \
  && sed -i 's/kvp("maxAwaitTimeMS", count)/kvp("maxAwaitTimeMS", static_cast<int64_t>(count))/' src/mongocxx/options/change_stream.cpp \
  && mkdir _build && cd _build \
  && cmake -DLIBBSON_DIR=/usr/local -DBOOST_ROOT=~/boost-build-1.69.0 \
    -DLIBMONGOC_DIR=/usr/local -DBSONCXX_POLY_USE_BOOST=1 \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .. \
  && make -j4 && make install \
  && cd - rm -rf mongo-cxx-driver.git

# Copy local directory to image
COPY . /catapult

# Build Catapult Server
RUN cd /catapult \
    && rm -rf _build \
    && mkdir _build && cd _build \
    && cmake -DCMAKE_BUILD_TYPE=Release  \
    -DCMAKE_CXX_FLAGS="-pthread -O3" \
    -DPYTHON_EXECUTABLE=/usr/bin/python3 \
    -DBSONCXX_LIB=/usr/local/lib/libbsoncxx.so \
    -DMONGOCXX_LIB=/usr/local/lib/libmongocxx.so \
    .. \
    && make publish \
    && make \
# Required extensions
    catapult.mongo.plugins.accountlink \
    catapult.mongo.plugins.aggregate \
    catapult.mongo.plugins.config \
    catapult.mongo.plugins.contract \
    catapult.mongo.plugins.exchange \
    catapult.mongo.plugins.lockhash \
    catapult.mongo.plugins.locksecret \
    catapult.mongo.plugins.metadata \
    catapult.mongo.plugins.mosaic \
    catapult.mongo.plugins.multisig \
    catapult.mongo.plugins.namespace \
    catapult.mongo.plugins.property \
    catapult.mongo.plugins.transfer \
    catapult.mongo.plugins.upgrade \
    catapult.mongo.plugins.service \
    catapult.plugins.accountlink \
    catapult.plugins.aggregate \
    catapult.plugins.config \
    catapult.plugins.contract \
    catapult.plugins.exchange \
    catapult.plugins.hashcache \
    catapult.plugins.hashcache.cache \
    catapult.plugins.lockhash \
    catapult.plugins.locksecret \
    catapult.plugins.metadata \
    catapult.plugins.mosaic \
    catapult.plugins.multisig \
    catapult.plugins.namespace \
    catapult.plugins.property \
    catapult.plugins.signature \
    catapult.plugins.transfer \
    catapult.plugins.upgrade \
    catapult.plugins.service \
    extension.addressextraction \
    extension.diagnostics \
    extension.eventsource \
    extension.filespooling \
    extension.harvesting \
    extension.hashcache \
    extension.mongo \
    extension.networkheight \
    extension.nodediscovery \
    extension.packetserver \
    extension.partialtransaction \
    extension.pluginhandlers \
    extension.sync \
    extension.syncsource \
    extension.timesync \
    extension.transactionsink \
    extension.unbondedpruning \
    extension.zeromq \
    -j 4

RUN cd /catapult/_build && make \
# Catapult
    sirius.bc \
    catapult.tools.address \
    catapult.tools.benchmark \
    catapult.tools.health \
    catapult.tools.nemgen \
    catapult.tools.nemgen.blockhashes \
    catapult.tools.network \
    catapult.tools.statusgen \
    -j 4

# script for copying required libs to temporary folder /deps
RUN /catapult/scripts/release-script/copyDeps.sh /catapult/_build/bin/ /deps

#------------------------
# Actual target image which only contains the required binaries
#------------------------
FROM ubuntu:16.04

# copy required libs
COPY --from=0 /deps /

# copy executables
COPY --from=0 /catapult/_build/bin /catapult/bin

# sirius.bc as the entry point
ENTRYPOINT ["/catapult/bin/sirius.bc"]

# set the config directory by default
CMD ["/catapultconfig"]
