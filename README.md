# Catapult server

[![docs](https://raw.githubusercontent.com/nemtech/catapult-server/master/badges/docs--green.svg)](https://nemtech.github.io)
[![docker](https://raw.githubusercontent.com/nemtech/catapult-server/master/badges/docker-techbureau-brightgreen.svg)](https://hub.docker.com/u/techbureau)

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
  To build a catapult.server image:
  ```
  cd proximax-catapult-server
  sudo ./scripts/Catapult.serverRealeaseDocker/buildCatapultServer.sh
  ```
