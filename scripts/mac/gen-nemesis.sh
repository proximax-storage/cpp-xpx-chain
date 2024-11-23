#!/bin/bash

# Copy addresses to cmake-build-debug
cd ~
copy /Users/alex/Proj/cpp-xpx-chain/addresses /Users/alex/Proj/cpp-xpx-chain/cmake-build-debug/addresses

# "Intel" - clear mongo
brew services stop mongodb/brew/mongodb-community
rm -rf /usr/local/var/mongodb/*
brew services start mongodb/brew/mongodb-community

# "Silicon" - clear mongo
brew services stop mongodb/brew/mongodb-community
rm -rf /opt/homebrew/var/mongodb/*
brew services start mongodb/brew/mongodb-community

# Generate nemesis
cd ~
./Proj/cpp-xpx-chain/scripts/bootstrap/runCatapultServers.sh

