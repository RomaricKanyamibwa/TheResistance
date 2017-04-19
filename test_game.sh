#! /bin/bash
#$IP_addr=134.157.104.21
#if [ "$#" -eq 1 ]; then
#	$IP_addr=$1
ip="$(ifconfig | grep -A 1 'eth0' | tail -1 | cut -d ':' -f 2 | cut -d ' ' -f 1)"
echo "Ip: $ip"
IP_addr=$ip
#./cmd.sh
#./mainserver 5 32000
./joueur $IP_addr 32000 $IP_addr 32001 Anna &
./joueur $IP_addr 32000 $IP_addr 32002 Jasmine &
./joueur $IP_addr 32000 $IP_addr 32003 Cole &
./joueur $IP_addr 32000 $IP_addr 32004 joe &
./joueur $IP_addr 32000 $IP_addr 32005 Pedro &

