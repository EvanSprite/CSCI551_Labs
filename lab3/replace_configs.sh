#!/bin/bash
mv ../lab1/router/IP_CONFIG ../lab1/router/IP_CONFIG_backup
cp ./lab1/IP_CONFIG ../lab1/router/
mv ../lab1/router/router/rtable ../lab1/router/router/rtable_backup
cp ./lab1/rtable ../lab1/router/router/
mv ../lab1/router/pox_module/cs144/ofhandler.py ../lab1/router/pox_module/cs144/ofhandler_backup.py
cp ./lab1/ofhandler.py ../lab1/router/pox_module/cs144/

mv ../lab2/miniNExT/start_internet2.py ../lab2/miniNExT/start_internet2_backup.py
cp ./lab2/start_internet2.py ../lab2/miniNExT/
mv ../lab2/http_server1 ../lab2/http_server1_backup 
cp -r ./lab2/http_server1 ../lab2/
mv ../lab2/http_server2 ../lab2/http_server2_backup
cp -r ./lab2/http_server2 ../lab2/

mv ../lab2/miniNExT/groups/ips.group ../lab2/miniNExT/groups/ips_backup.group
cp ./lab2/ips.group ../lab2/miniNExT/groups
