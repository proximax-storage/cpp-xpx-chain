FROM debian:10-slim

RUN apt-get update && apt-get install --no-install-recommends -y \
  libboost-atomic1.67.0 \
  libboost-chrono1.67.0 \
  libboost-date-time1.67.0 \
  libboost-regex1.67.0 \
  libboost-system1.67.0 \
  libboost-timer1.67.0 \
  libboost-log1.67.0 \
  libboost-thread1.67.0 \
  libboost-program-options1.67.0 \
  libboost-random1.67.0 \
  librocksdb5.17 \
  cmake

# Install mongo-cxx
RUN curl -OL https://github.com/mongodb/mongo-cxx-driver/archive/r3.4.0.tar.gz && \
  tar xzf r3.4.0.tar.gz && \
  cd mongo-cxx-driver-r3.4.0 && \
  sed -i 's/kvp("maxAwaitTimeMS", count)/kvp("maxAwaitTimeMS", static_cast<int64_t>(count))/' src/mongocxx/options/change_stream.cpp && \
  cd build && \
  cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DBSONCXX_POLY_USE_BOOST=1 .. && \
  make -j4 && \
  make install

# Clean Downloads
RUN rm r3.4.0.tar.gz

COPY _build/bin /sirius/bin

ENTRYPOINT [ "/sirius/bin/sirius.bc" ]

CMD [ "/chainconfig"]

