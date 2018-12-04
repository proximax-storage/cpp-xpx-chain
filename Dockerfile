FROM ubuntu:16.04

# copy required libs
COPY ./deps /

# copy executables
COPY ./_build/bin /catapult/bin

# catapult.server as the entry point
ENTRYPOINT ["/catapult/bin/catapult.server"]

# set the config directory by default
CMD ["/catapultconfig"]
