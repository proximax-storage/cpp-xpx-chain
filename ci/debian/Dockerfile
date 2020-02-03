FROM debian:10-slim

COPY ./deps /

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
  librocksdb5.17 

COPY _build/bin /sirius/bin

ENTRYPOINT [ "/sirius/bin/sirius.bc" ]

CMD [ "/chainconfig"]
