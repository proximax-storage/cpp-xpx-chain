#!/bin/bash

#"Silicon"

brew services stop mongodb/brew/mongodb-community
rm -rf /opt/homebrew/var/mongodb/*
brew services start mongodb/brew/mongodb-community
