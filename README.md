[![docs](badges/docs--green.svg)](https://bcdocs.xpxsirius.io)

# ProximaX Sirius-Chain Server Code #

Official ProximaX Sirius-Chain Server Code.

The ProximaX Sirius-Chain Server code is the server code implementation of ProximaX blockchain layer.

## Building
Detailed building instructions are [here](BUILDING.md).

## Contributing
Before contributing please [read this](CONTRIBUTING.md).

To create a release docker images you need to install docker first:
```
sudo apt-get install docker-compose -y
```
  Then you need to build an image:
  To build a tools image:
  ```
  cd cpp-xpx-chain
  sudo ./scripts/ToolsRealeaseDocker/buildTools.sh
  ```
  To build a catapult.server image:
  ```
  cd cpp-xpx-chain
  sudo ./scripts/Catapult.serverRealeaseDocker/buildCatapultServer.sh
  ```

## Sanitizers

Sanitizers are compiler flags, that inject verification code into binary. [Detailed](https://github.com/google/sanitizers).
Once can build PromimaX with one of sanitizers enabled.

```
# 1. do a clean build with one of sanitizers enabled
cmake .. -DSANITIZE_ADDRESS=ON    # enables address & memory leak sanitizers
cmake .. -DSANITIZE_THREAD=ON     # enables thread sanitizer
cmake .. -DSANITIZE_UNDEFINED=ON  # enables undefined behavior sanitizer
# 2. build
make
# 3. run binary OR tests
ctest
```

If binary contains errors, it will fail with detailed stacktrace.
