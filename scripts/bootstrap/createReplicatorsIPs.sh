#!/bin/bash

name=$(ip link | awk -F: '$0 ~ "eth*"{print $2;getline}')
if [ -z "$var" ];
then
	name=$(ip link | awk -F: '$0 ~ "wl*"{print $2;getline}')
fi

printf "Chosen network interface: $name\n"
printf "Creating addresses...\n"

client=192.168.20.30
printf "Client: $client\n"
sudo ip addr add $client dev $name

api=192.168.10.10
apiNet=apiNetwork
printf "Api node: $api\n"
sudo ip link add $apiNet type dummy
sudo ip addr add $api dev $apiNet
sudo ip link set $apiNet up

declare -a peers=("192.168.20.20" "192.168.20.21" "192.168.20.22" "192.168.20.23" "192.168.20.24")
for (( i=0; i<${#peers[@]}; i++ ));
do
	printf "Peer node/replicator $i: ${peers[$i]}\n"
	sudo ip link add replicator$i type dummy
  sudo ip addr add ${peers[$i]} dev replicator$i
  sudo ip link set replicator$i up
done
