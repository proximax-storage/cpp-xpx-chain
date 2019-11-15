FROM ubuntu:16.04

# copy required libs
COPY ./deps /

# copy executables
COPY ./_build/bin /catapult/bin

# delete the Tools
RUN rm -rf /catapult/bin/catapult.tools.*

# sirius.bc as the entry point
ENTRYPOINT ["/catapult/bin/sirius.bc"]

# set the config directory by default
CMD ["/catapultconfig"]
