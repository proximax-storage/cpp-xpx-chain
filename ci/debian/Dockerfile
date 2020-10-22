FROM debian:unstable-20200607

COPY ./deps /

RUN apt-get update && apt-get install --no-install-recommends -y \
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
  libzstd-dev \
  librocksdb5.17 

COPY _build/bin /sirius/bin

ENTRYPOINT [ "/sirius/bin/sirius.bc" ]

CMD [ "/chainconfig"]
