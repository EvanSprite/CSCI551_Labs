#!/bin/bash

# Delete all the VMS except the main one

if [ $# -ne 1 ]; then
    echo $0: usage ./make_vms main_vm
    exit 1
fi

main_vm=$1

# Stop the VMs that must be created but that are running
for name in $(vboxmanage list vms | cut -f 1 -d ' ' | tr -d \"); do
    if [ "$name" != "$main_vm" ]; then
        echo $name
        vboxmanage controlvm $name poweroff
        vboxmanage unregistervm $name --delete
    fi
done
