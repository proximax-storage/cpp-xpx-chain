#!/bin/bash

cd /catapult/_build && cmake .. && make -j$(nproc)