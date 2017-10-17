#!/bin/bash

ifconfig eth1 192.168.56.199 up

ovs-vsctl del-br MGT
ovs-vsctl add-br MGT
echo '' > .make_tunnels.sh

for i in {1..40}
do
    echo 'Configuring connection to AS'$i
    ip link del g$i-ovs type veth peer name g$i
    ip link add g$i-ovs type veth peer name g$i

    sudo ifconfig g$i-ovs up
    sudo ifconfig g$i $i.0.199.2/24

    ovs-vsctl add-port MGT g$i-ovs -- set Interface g$i-ovs ofport=$i

    ./create_tunnels.sh $i
done
