FROM ubuntu:22.04

# copy required libs
COPY ./deps /

# copy executables
COPY ./_build/bin /sirius/bin

# sirius.bc as the entry point
ENTRYPOINT ["/sirius/bin/sirius.bc"]

# set the config directory by default
CMD ["/chainconfig"]
