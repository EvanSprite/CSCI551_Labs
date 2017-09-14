echo 'File created on '$(date) > /root/.sh_ip_bgp/$(cat /root/.prompt).txt
vtysh -c "show ip bgp"  >> /root/.sh_ip_bgp/$(cat /root/.prompt).txt
