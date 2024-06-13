#!/bin/bash

name="en0"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
  name="$(ip link | awk -F: '$0 ~ "eth*"{print $2;getline}')"
elif [ -z "$name" ];
then
	name=$(ip link | awk -F: '$0 ~ "wl"{print $2;getline}')
fi

printf "Chosen network interface: $name\n"
printf "Creating addresses...\n"

client=192.168.20.30
printf "Client: $client\n"
sudo ip addr add $client dev $name

api=192.168.10.10
printf "Api node: $api\n"
sudo ip addr add $api dev $name

declare -a peers=("192.168.20.20" "192.168.20.21" "192.168.20.22" "192.168.20.23" "192.168.20.24")
#if [[ "$OSTYPE" == "darwin"* ]]; then
#  declare -a peers=("192.168.30.10" "192.168.31.10" "192.168.32.10" "192.168.33.10" "192.168.34.10")
#fi

for (( i=0; i<${#peers[@]}; i++ ));
do
	printf "Peer node/replicator $i: ${peers[$i]}\n"
	sudo ip addr add ${peers[$i]} dev $name
done
