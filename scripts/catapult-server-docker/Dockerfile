FROM ubuntu:20.04

WORKDIR /tmp

# tzdata settings
ENV TZ=Europe/Kiev
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

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

# gcc,g++ 10
RUN add-apt-repository ppa:ubuntu-toolchain-r/test \
  && apt-get update && apt-get install -y --no-install-recommends gcc-10 g++-10 \
  && apt-get clean && rm -rf /var/lib/apt/lists/* \
  && rm /usr/bin/gcc /usr/bin/g++ \
  && ln -s /usr/bin/gcc-10 /usr/bin/gcc \
  && ln -s /usr/bin/g++-10 /usr/bin/g++

# cmake
RUN git clone https://gitlab.kitware.com/cmake/cmake.git -b v3.24.2 --depth 1 \
  && cd cmake \
  && ./bootstrap --prefix=/usr/local && make -j22 && make install \
  && cd -

# boost
RUN wget https://boostorg.jfrog.io/artifactory/main/release/1.71.0/source/boost_1_71_0.tar.gz \
  && tar xzf boost_1_71_0.tar.gz && cd boost_1_71_0 \
  && ./bootstrap.sh && ./b2 toolset=gcc install --prefix=/usr/local -j22 \
  && cd - && rm boost_1_71_0.tar.gz && rm -rf boost_1_71_0.tar.gz

# gtest
RUN git clone https://github.com/google/googletest.git -b release-1.8.1 --depth 1 \
  && mkdir -p googletest/_build && cd googletest/_build \
  && cmake -DCMAKE_INSTALL_PREFIX=/usr/local .. && make -j22 && make install \
  && cd -

# benchmark
RUN git clone https://github.com/google/benchmark.git google.benchmark.git -b v1.4.1 --depth 1 \
  && cd google.benchmark.git && mkdir _build && cd _build \
  && cmake -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE_GTEST_TESTS=OFF .. && make -j22 && make install \
  && cd - && rm -rf google.benchmark.git

# rocksdb
RUN git clone https://github.com/facebook/rocksdb.git -b v6.6.4 --depth 1 \
  && mkdir -p rocksdb/_build && cd rocksdb/_build \
  && cmake -DCMAKE_INSTALL_PREFIX=/usr/local .. && make -j22 && make install \
  && cd -

# zmqlib
RUN git clone https://github.com/zeromq/libzmq.git -b v4.3.2 --depth 1 \
  && mkdir -p libzmq/_build && cd libzmq/_build \
  && cmake -DCMAKE_INSTALL_PREFIX=/usr/local .. && make -j22 && make install \
  && cd -

# cppzmq
RUN git clone https://github.com/zeromq/cppzmq.git -b v4.4.1 --depth 1 \
  && mkdir -p cppzmq/_build && cd cppzmq/_build \
  && cmake -DCPPZMQ_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release -DCPPZMQ_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local ..  \
  && make -j22 && make install \
  && cd -

# mongo-c
RUN git clone https://github.com/mongodb/mongo-c-driver.git mongo-c-driver.git -b 1.20.0 --depth 1 && cd mongo-c-driver.git \
  && mkdir _build && cd _build \
  && cmake -DPYTHON_EXECUTABLE=/usr/bin/python3 -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .. \
  && make -j22 && make install \
  && cd - && rm -rf mongo-c-driver.git

# mongo-cxx
RUN git clone https://github.com/mongodb/mongo-cxx-driver.git mongo-cxx-driver.git -b r3.6.0 --depth 1 && cd mongo-cxx-driver.git \
  && sed -i 's/kvp("maxAwaitTimeMS", count)/kvp("maxAwaitTimeMS", static_cast<int64_t>(count))/' src/mongocxx/options/change_stream.cpp \
  && mkdir _build && cd _build \
  && cmake -DLIBBSON_DIR=/usr/local -DBOOST_ROOT=~/boost-build-1.71.0 \
    -DPYTHON_EXECUTABLE=/usr/bin/python3 \
    -DLIBMONGOC_DIR=/usr/local -DBSONCXX_POLY_USE_BOOST=1 \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .. \
  && make -j22 && make install \
  && cd - rm -rf mongo-cxx-driver.git

# Copy local directory to image
COPY . /catapult

# Build Catapult Server
RUN cd /catapult \
    && rm -rf _build \
    && mkdir _build && cd _build \
    && cmake -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-pthread -O3" \
    -DPYTHON_EXECUTABLE=/usr/bin/python3 \
    -DBSONCXX_LIB=/usr/local/lib/libbsoncxx.so \
    -DMONGOCXX_LIB=/usr/local/lib/libmongocxx.so \
    -DBOOST_ROOT=~/boost-build-1.71.0 \
    .. \
    && make publish \
    && make \
# Required extensions
    catapult.mongo.plugins.accountlink \
    catapult.mongo.plugins.aggregate \
    catapult.mongo.plugins.committee \
    catapult.mongo.plugins.config \
    catapult.mongo.plugins.contract \
    catapult.mongo.plugins.exchange \
    catapult.mongo.plugins.exchangesda \
    catapult.mongo.plugins.liquidityprovider \
    catapult.mongo.plugins.lockhash \
    catapult.mongo.plugins.locksecret \
    catapult.mongo.plugins.metadata \
    catapult.mongo.plugins.metadata_v2 \
    catapult.mongo.plugins.mosaic \
    catapult.mongo.plugins.multisig \
    catapult.mongo.plugins.namespace \
    catapult.mongo.plugins.operation \
    catapult.mongo.plugins.property \
    catapult.mongo.plugins.transfer \
    catapult.mongo.plugins.upgrade \
    catapult.mongo.plugins.service \
    catapult.mongo.plugins.storage \
    catapult.mongo.plugins.streaming \
    catapult.mongo.plugins.supercontract \
    catapult.plugins.accountlink \
    catapult.plugins.aggregate \
    catapult.plugins.committee \
    catapult.plugins.config \
    catapult.plugins.contract \
    catapult.plugins.exchange \
    catapult.plugins.exchangesda \
    catapult.plugins.liquidityprovider \
    catapult.plugins.hashcache \
    catapult.plugins.hashcache.cache \
    catapult.plugins.lockhash \
    catapult.plugins.locksecret \
    catapult.plugins.metadata \
    catapult.plugins.metadata_v2 \
    catapult.plugins.mosaic \
    catapult.plugins.multisig \
    catapult.plugins.namespace \
    catapult.plugins.operation \
    catapult.plugins.property \
    catapult.plugins.signature \
    catapult.plugins.transfer \
    catapult.plugins.upgrade \
    catapult.plugins.service \
    catapult.plugins.storage \
    catapult.plugins.streaming \
    catapult.plugins.supercontract \
    extension.addressextraction \
    extension.diagnostics \
    extension.eventsource \
    extension.fastfinality \
    extension.filespooling \
    extension.harvesting \
    extension.hashcache \
    extension.mongo \
    extension.networkheight \
    extension.nodediscovery \
    extension.packetserver \
    extension.partialtransaction \
    extension.pluginhandlers \
    extension.storage \
    extension.sync \
    extension.syncsource \
    extension.timesync \
    extension.transactionsink \
    extension.unbondedpruning \
    extension.zeromq \
    -j 22

RUN cd /catapult/_build && make \
# Catapult
    sirius.bc \
    catapult.broker \
    catapult.recovery \
    catapult.tools.address \
    catapult.tools.benchmark \
    catapult.tools.health \
    catapult.tools.nemgen \
    catapult.tools.nemgen.blockhashes \
    catapult.tools.network \
    catapult.tools.statusgen \
    -j 22

# script for copying required libs to temporary folder /deps
RUN /catapult/scripts/release-script/copyDeps.sh /catapult/_build/bin/ /deps

#------------------------
# Actual target image which only contains the required binaries
#------------------------
FROM ubuntu:20.04

# copy required libs
COPY --from=0 /deps /

# copy executables
COPY --from=0 /catapult/_build/bin /catapult/bin

# sirius.bc as the entry point
ENTRYPOINT ["/catapult/bin/sirius.bc"]

# set the config directory by default
CMD ["/catapultconfig"]
