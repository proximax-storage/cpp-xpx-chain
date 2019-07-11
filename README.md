# Catapult server

[![docs](badges/docs--green.svg)](https://nemtech.github.io)
[![docker](badges/docker-techbureau-brightgreen.svg)](https://hub.docker.com/u/techbureau)

## Welcome

Catapult server is upcoming node server of NEM project.

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
  cd proximax-catapult-server
  sudo ./scripts/ToolsRealeaseDocker/buildTools.sh
  ```
  To build a sirius.bc image:
  ```
  cd cpp-xpx-chain
  sudo ./scripts/release-script/buildCatapultServer.sh
  ```
