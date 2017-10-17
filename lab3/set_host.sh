sudo ./go_to.sh CHIC-host ifconfig chic 2.102.0.1/24
sudo ./go_to.sh CHIC-host route add default gw 2.102.0.2
sudo ./go_to.sh WASH-host ifconfig  wash 2.103.0.1/24
sudo ./go_to.sh WASH-host route add default gw 2.103.0.2
sudo ./go_to.sh ATLA-host ifconfig atla 2.104.0.1/24
sudo ./go_to.sh ATLA-host route add default gw 2.104.0.2
sudo ./go_to.sh KANS-host ifconfig kans 2.105.0.1/24
sudo ./go_to.sh KANS-host route add default gw 2.105.0.2
sudo ./go_to.sh HOUS-host ifconfig hous 2.106.0.1/24
sudo ./go_to.sh HOUS-host route add default gw 2.106.0.2
sudo ./go_to.sh SALT-host ifconfig salt 2.107.0.1/24
sudo ./go_to.sh SALT-host route add default gw 2.107.0.2
sudo ./go_to.sh LOSA-host ifconfig losa 2.108.0.1/24
sudo ./go_to.sh LOSA-host route add default gw 2.108.0.2
sudo ./go_to.sh SEAT-host ifconfig seat 2.109.10.1/24
sudo ./go_to.sh SEAT-host route add default gw 2.109.10.2
