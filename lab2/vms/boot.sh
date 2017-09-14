#!/bin/bash

if pgrep "zebra" > /dev/null
then
	echo "miniNExT is running. Just run ./go_to ROUTER_NAME to go to the ROUTER_NAME host."
else
	#REPLY=''

    #sudo killall ovs-controller > /home/thomas/debug 2>&1

	#echo "Starting miniNExT..."
	#while [ "$REPLY" != "y" -a "$REPLY" != "n" ]; do
	#	read -p "Do you want to reset your Quagga config files? [y/n]:" -n 1 -r

	#	if [ "$REPLY" == "y" ]; then
	#		tmux kill-session -t mininext
	#		tmux new-session -d -s mininext
	#		tmux send -t mininext "sudo python /root/miniNExT/examples/comm_network/start_internet2.py /root/miniNExT/examples/comm_network/groups/ips.group --new" ENTER
	#	elif [ "$REPLY" == "n" ]; then
			#tmux kill-session -t mininext
			#tmux new-session -d -s mininext
			#tmux send -t mininext "sudo python /root/miniNExT/examples/comm_network/start_internet2.py /root/miniNExT/examples/comm_network/groups/ips.group" ENTER
	#	fi
	#done
	#echo "miniNExT is running. Just run ./go_to ROUTER_NAME to go to the ROUTER_NAME host."
    echo 'miniNExT is not running. This is not good. Please report this to your teaching assistant.'
fi
