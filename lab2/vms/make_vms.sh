#!/bin/bash

if [ $# -ne 2 ]; then
    echo $0: usage ./make_vms main_vm filename
    exit 1
fi

main_vm=$1

# Stop the main VM
vboxmanage controlvm $main_vm poweroff

# Stop and delete the VMs that must be created but that are running
#while read name passwd sshport; do
    # Clone the main VM
#    vboxmanage controlvm $name poweroff
#    vboxmanage unregistervm $name --delete
#done < $2


while read name passwd sshport intf_ip; do
    # Clone the main VM
    vboxmanage clonevm $main_vm --name $name --register

    # Remove any possible NAT rules for ssh
    echo vboxmanage modifyvm $name --natpf1 delete ssh
    vboxmanage modifyvm $name --natpf1 delete ssh
    # Set new NAT rule for remote access
    echo vboxmanage modifyvm $name --natpf1 ssh,tcp,,$sshport,10.0.2.15,22
    vboxmanage modifyvm $name --natpf1 ssh,tcp,,$sshport,10.0.2.15,22

    # Start the VM
    echo Start VM $name
    vboxmanage startvm $name --type headless

    sleep 30
done < $2
