FROM ubuntu:16.04

# copy required libs
COPY ./deps /

# copy executables
COPY ./_build/bin /catapult/bin

# sirius.bc as the entry point
ENTRYPOINT ["/catapult/bin/sirius.bc"]

# set the config directory by default
CMD ["/catapultconfig"]
