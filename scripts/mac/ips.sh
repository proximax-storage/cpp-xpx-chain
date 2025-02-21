#!/bin/bash


sudo networksetup -createnetworkservice "Wi-Fi 20" en1
sudo networksetup -createnetworkservice "Wi-Fi 21" en1
sudo networksetup -createnetworkservice "Wi-Fi 22" en1
sudo networksetup -createnetworkservice "Wi-Fi 23" en1
sudo networksetup -createnetworkservice "Wi-Fi 24" en1

sudo networksetup -createnetworkservice "Wi-Fi 30" en1
sudo networksetup -createnetworkservice "Wi-Fi 31" en1
sudo networksetup -createnetworkservice "Wi-Fi 32" en1

sudo networksetup -setmanual "Wi-Fi 20" 192.168.20.20 255.255.255.0 192.168.1.1
sudo networksetup -setmanual "Wi-Fi 21" 192.168.20.21 255.255.255.0 192.168.1.1
sudo networksetup -setmanual "Wi-Fi 22" 192.168.20.22 255.255.255.0 192.168.1.1
sudo networksetup -setmanual "Wi-Fi 23" 192.168.20.23 255.255.255.0 192.168.1.1
sudo networksetup -setmanual "Wi-Fi 24" 192.168.20.24 255.255.255.0 192.168.1.1

sudo networksetup -setmanual "Wi-Fi 30" 192.168.20.30 255.255.255.0 192.168.1.1
sudo networksetup -setmanual "Wi-Fi 31" 192.168.20.31 255.255.255.0 192.168.1.1
sudo networksetup -setmanual "Wi-Fi 32" 192.168.20.32 255.255.255.0 192.168.1.1

networksetup -listallnetworkservices
###############################################################
exit

#IFACE="en1"
IFACE="lo0"

# peers
sudo ifconfig $IFACE alias 192.168.20.20/16 up
sudo ifconfig $IFACE alias 192.168.20.21/16 up
sudo ifconfig $IFACE alias 192.168.20.22/16 up
sudo ifconfig $IFACE alias 192.168.20.23/16 up
sudo ifconfig $IFACE alias 192.168.20.24/16 up

# clients
sudo ifconfig $IFACE alias 192.168.20.30/16 up
sudo ifconfig $IFACE alias 192.168.20.31/16 up
sudo ifconfig $IFACE alias 192.168.20.32/16 up


