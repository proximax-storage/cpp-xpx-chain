modprobe dummy

ip link add eth10 type dummy
ip address change dev eth10 192.168.10.10

ip link add eth20 type dummy
ip address change dev eth20 192.168.20.20

ip link add eth21 type dummy
ip address change dev eth21 192.168.20.21

ip link add eth22 type dummy
ip address change dev eth22 192.168.20.22

ip link add eth23 type dummy
ip address change dev eth23 192.168.20.23

ip link add eth24 type dummy
ip address change dev eth24 192.168.20.24
