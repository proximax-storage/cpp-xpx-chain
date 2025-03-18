#!/bin/bash

sudo networksetup -removenetworkservice "Wi-Fi 20"
sudo networksetup -removenetworkservice "Wi-Fi 21"
sudo networksetup -removenetworkservice "Wi-Fi 22"
sudo networksetup -removenetworkservice "Wi-Fi 23"
sudo networksetup -removenetworkservice "Wi-Fi 24"

sudo networksetup -removenetworkservice "Wi-Fi 30"
sudo networksetup -removenetworkservice "Wi-Fi 31"
sudo networksetup -removenetworkservice "Wi-Fi 32"

networksetup -listallnetworkservices
exit
################################################################

IFACE="lo0"

# peers
sudo ifconfig $IFACE -alias 192.168.20.20
sudo ifconfig $IFACE -alias 192.168.20.21
sudo ifconfig $IFACE -alias 192.168.20.22
sudo ifconfig $IFACE -alias 192.168.20.23
sudo ifconfig $IFACE -alias 192.168.20.24

# clients
sudo ifconfig $IFACE -alias 192.168.20.30
sudo ifconfig $IFACE -alias 192.168.20.31
sudo ifconfig $IFACE -alias 192.168.20.32


