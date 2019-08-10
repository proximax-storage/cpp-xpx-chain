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
