
ovs-vsctl add-port EBGP_PEER gre_to_22 -- set interface gre_to_22 type=gre options:remote_ip=192.168.56.122
port_in=`ovs-ofctl show EBGP_PEER | grep wash-ovs | cut -f 1 -d '(' | tr -d ' '`
port_out=`ovs-ofctl show EBGP_PEER | grep \(gre_to_22\) | cut -f 1 -d '(' | tr -d ' '`
ovs-ofctl add-flow EBGP_PEER in_port=$port_in,actions=output:$port_out
ovs-ofctl add-flow EBGP_PEER in_port=$port_out,actions=output:$port_in
ovs-vsctl add-port EBGP_PEER gre_to_25 -- set interface gre_to_25 type=gre options:remote_ip=192.168.56.125
port_in=`ovs-ofctl show EBGP_PEER | grep salt-ovs | cut -f 1 -d '(' | tr -d ' '`
port_out=`ovs-ofctl show EBGP_PEER | grep \(gre_to_25\) | cut -f 1 -d '(' | tr -d ' '`
ovs-ofctl add-flow EBGP_PEER in_port=$port_in,actions=output:$port_out
ovs-ofctl add-flow EBGP_PEER in_port=$port_out,actions=output:$port_in
ovs-vsctl add-port EBGP_PEER gre_to_26 -- set interface gre_to_26 type=gre options:remote_ip=192.168.56.126
port_in=`ovs-ofctl show EBGP_PEER | grep kans-ovs | cut -f 1 -d '(' | tr -d ' '`
port_out=`ovs-ofctl show EBGP_PEER | grep \(gre_to_26\) | cut -f 1 -d '(' | tr -d ' '`
ovs-ofctl add-flow EBGP_PEER in_port=$port_in,actions=output:$port_out
ovs-ofctl add-flow EBGP_PEER in_port=$port_out,actions=output:$port_in
ovs-vsctl add-port EBGP_PEER gre_to_30 -- set interface gre_to_30 type=gre options:remote_ip=192.168.56.130
port_in=`ovs-ofctl show EBGP_PEER | grep losa-ovs | cut -f 1 -d '(' | tr -d ' '`
port_out=`ovs-ofctl show EBGP_PEER | grep \(gre_to_30\) | cut -f 1 -d '(' | tr -d ' '`
ovs-ofctl add-flow EBGP_PEER in_port=$port_in,actions=output:$port_out
ovs-ofctl add-flow EBGP_PEER in_port=$port_out,actions=output:$port_in
ovs-vsctl add-port EBGP_PEER gre_to_16 -- set interface gre_to_16 type=gre options:remote_ip=192.168.56.116
port_in=`ovs-ofctl show EBGP_PEER | grep newy-ovs | cut -f 1 -d '(' | tr -d ' '`
port_out=`ovs-ofctl show EBGP_PEER | grep \(gre_to_16\) | cut -f 1 -d '(' | tr -d ' '`
ovs-ofctl add-flow EBGP_PEER in_port=$port_in,actions=output:$port_out
ovs-ofctl add-flow EBGP_PEER in_port=$port_out,actions=output:$port_in
ovs-vsctl add-port EBGP_PEER gre_to_17 -- set interface gre_to_17 type=gre options:remote_ip=192.168.56.117
port_in=`ovs-ofctl show EBGP_PEER | grep seat-ovs | cut -f 1 -d '(' | tr -d ' '`
port_out=`ovs-ofctl show EBGP_PEER | grep \(gre_to_17\) | cut -f 1 -d '(' | tr -d ' '`
ovs-ofctl add-flow EBGP_PEER in_port=$port_in,actions=output:$port_out
ovs-ofctl add-flow EBGP_PEER in_port=$port_out,actions=output:$port_in
ovs-vsctl add-port EBGP_PEER gre_to_99 -- set interface gre_to_99 type=gre options:remote_ip=192.168.56.199
port_in=`ovs-ofctl show EBGP_PEER | grep hous-mgt-ovs | cut -f 1 -d '(' | tr -d ' '`
port_out=`ovs-ofctl show EBGP_PEER | grep \(gre_to_99\) | cut -f 1 -d '(' | tr -d ' '`
ovs-ofctl add-flow EBGP_PEER in_port=$port_in,actions=output:$port_out
ovs-ofctl add-flow EBGP_PEER in_port=$port_out,actions=output:$port_in
