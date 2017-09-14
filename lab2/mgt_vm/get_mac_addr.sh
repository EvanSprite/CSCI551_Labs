#!/bin/bash

while true

do
    echo "" > mac_addresses
    echo "# File generated "`date` >> mac_addresses

        for i in {1..40}
        do
            route add default gw $i.0.199.1
            ping $i.0.199.1 -c 1 -t 1 > /dev/null 2>&1
            echo G$i `arp -n $i.0.199.1 | grep $i.0.199.1 | tr -s " " | cut -f 3 -d ' '` >> mac_addresses
            route del default gw $i.0.199.1
        done
        sleep 3600
done
