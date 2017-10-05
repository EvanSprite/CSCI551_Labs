#!/bin/bash

if [ $# -ne 1 ]; then
    echo $0: usage ./.start_mininext.sh group_number
    exit 1
fi

grp_nb=$1

sudo killall ovs-controller > /home/cs551/debug 2>&1
tmux kill-session -t mininext
tmux new-session -d -s mininext
tmux send -t mininext "sudo python /home/cs551/evansprite-cs551/lab2/miniNExT/start_internet2.py $grp_nb /home/cs551/evansprite-cs551/lab2/miniNExT/groups/ips.group --new" ENTER
