#!/bin/bash

# api/rest node
sudo ifconfig lo0 alias 192.168.10.10/16 up

# peers
sudo ifconfig lo0 alias 192.168.20.20/16 up
sudo ifconfig lo0 alias 192.168.20.21/16 up
sudo ifconfig lo0 alias 192.168.20.22/16 up
sudo ifconfig lo0 alias 192.168.20.23/16 up
sudo ifconfig lo0 alias 192.168.20.24/16 up

# clients
sudo ifconfig lo0 alias 192.168.20.30/16 up
sudo ifconfig lo0 alias 192.168.20.31/16 up
sudo ifconfig lo0 alias 192.168.20.32/16 up


