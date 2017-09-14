#!/bin/bash

# Script used to create tunnels between VMs. This script needs to be run on the VM.

if [ $# -ne 2 ]; then
    echo $0: usage ./make_vms dst_grp intf
    exit 1
fi

dst_grp=$1
dst_ip=192.168.56.$((100+$dst_grp))
intf=`echo $2 | tr '[:upper:]' '[:lower:]'`

echo ovs-vsctl add-port EBGP_PEER gre_to_$dst_grp -- set interface gre_to_$dst_grp type=gre options:remote_ip=192.168.56.$((100+$dst_grp)) >> .make_tunnels.sh

echo port_in="\`ovs-ofctl show EBGP_PEER | grep $intf-ovs | cut -f 1 -d '(' | tr -d ' '\`" >> .make_tunnels.sh
echo port_out="\`ovs-ofctl show EBGP_PEER | grep \(gre_to_$dst_grp\) | cut -f 1 -d '(' | tr -d ' '\`" >> .make_tunnels.sh

echo ovs-ofctl add-flow EBGP_PEER in_port=\$port_in,actions=output:\$port_out >> .make_tunnels.sh
echo ovs-ofctl add-flow EBGP_PEER in_port=\$port_out,actions=output:\$port_in >> .make_tunnels.sh
