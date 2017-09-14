#!/bin/bash

while true
do
        scp -i ~/.ssh/id_rsa -P 3099 root@samichlaus.ethz.ch:~/.matrix/matrix.html /var/www/comm-net/html/cgi/

        for i in {1..10}
        do
                for router in NEWY CHIC ATLA WASH KANS HOUS SALT LOSA SEAT; do
                scp -i ~/.ssh/id_rsa -P $((3000+$i)) root@samichlaus.ethz.ch:/root/.sh_ip_bgp/${router}.txt /var/www/comm-net/html/cgi/G${i}_${router}.txt &
                done
        done
        sleep 10

        scp -i ~/.ssh/id_rsa -P 3099 root@samichlaus.ethz.ch:~/.matrix/matrix.html /var/www/comm-net/html/cgi/

        for i in {11..20}
        do
                for router in NEWY CHIC ATLA WASH KANS HOUS SALT LOSA SEAT; do
                scp -i ~/.ssh/id_rsa -P $((3000+$i)) root@samichlaus.ethz.ch:/root/.sh_ip_bgp/${router}.txt /var/www/comm-net/html/cgi/G${i}_${router}.txt &
                done
        done
        sleep 10

        scp -i ~/.ssh/id_rsa -P 3099 root@samichlaus.ethz.ch:~/.matrix/matrix.html /var/www/comm-net/html/cgi/

        for i in {21..30}
        do
                for router in NEWY CHIC ATLA WASH KANS HOUS SALT LOSA SEAT; do
                scp -i ~/.ssh/id_rsa -P $((3000+$i)) root@samichlaus.ethz.ch:/root/.sh_ip_bgp/${router}.txt /var/www/comm-net/html/cgi/G${i}_${router}.txt &
                done
        done
        sleep 10

        scp -i ~/.ssh/id_rsa -P 3099 root@samichlaus.ethz.ch:~/.matrix/matrix.html /var/www/comm-net/html/cgi/

        for i in {31..40}
        do
                for router in NEWY CHIC ATLA WASH KANS HOUS SALT LOSA SEAT; do
                scp -i ~/.ssh/id_rsa -P $((3000+$i)) root@samichlaus.ethz.ch:/root/.sh_ip_bgp/${router}.txt /var/www/comm-net/html/cgi/G${i}_${router}.txt &
                done
        done
        sleep 10
done
