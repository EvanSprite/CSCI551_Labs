#!/bin/bash

if [ $# -ne 2 ]; then
    echo $0: usage ./make_vms groups_info connections_info
    exit 1
fi

# Start the virtual network
while read name passwd sshport ip; do
    grp_nb=$(($sshport-3000)) # Port number starts at 3001

    echo fab start_mininext:dst_ip=localhost,port_number=$sshport,group_number=$grp_nb
    fab start_mininext:dst_ip=localhost,port_number=$sshport,group_number=$grp_nb &
done < $1

sleep 30
# Set the IP address for eth1
while read name passwd sshport ip; do
    echo fab set_ip:dst_ip=localhost,port_number=$sshport,ip_eth1=$ip
    fab set_ip:dst_ip=localhost,port_number=$sshport,ip_eth1=$ip &
done < $1

sleep 30
# Clean the make_tunnels script
while read osef1 osef2 grpid dst_vm intf osef3; do
    sshport=$((3000+grpid)) # Port number starts at 3001

    echo fab clean_make_tunnels:dst_ip=localhost,port_number=$sshport
    fab clean_make_tunnels:dst_ip=localhost,port_number=$sshport &
done < $2

sleep 30
# Configure the tunnels between the VMs
while read osef1 osef2 grpid dst_vm intf osef3; do
    sshport=$((3000+grpid)) # Port number starts at 3001

    echo fab make_vm_connection:dst_ip=localhost,port_number=$sshport,dst_vm=$dst_vm,intf=$intf
    fab make_vm_connection:dst_ip=localhost,port_number=$sshport,dst_vm=$dst_vm,intf=$intf &
done < $2

sleep 30
# Configure the tunnels
while read name passwd sshport ip; do
    echo fab make_tunnels:dst_ip=localhost,port_number=$sshport
    fab make_tunnels:dst_ip=localhost,port_number=$sshport &
done < $1

sleep 30
 Configure the passwd
while read name passwd sshport ip; do
    echo fab set_cred:dst_ip=localhost,port_number=$sshport,new_passwd=$passwd
    fab set_cred:dst_ip=localhost,port_number=$sshport,new_passwd=$passwd &
done < $1
