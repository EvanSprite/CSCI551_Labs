"""
Generates a file containing all the configuration commands for all the hosts and routers of one group.
Currently only the router interfaces, OSPF and iBGP sessions are configured (also the mgt interface).
The eBGP sessions are also prepared.
=> The ip address for the ebgp_peer interface as well as the neighbor command for the eBGP peer have to be added manually!
Copy and paste the file content into the home directory of the corresponding group.
IMPORTANT: old router or host configurations are currently not removed.
           Therefore, use the generated configuration only on a "clean" VM (e.g. after a reboot).
"""

import argparse

# router name <--> router number
name_dict = {1: 'NEWY', 2: 'CHIC', 3: 'WASH', 4: 'ATLA', 5: 'KANS', 6: 'HOUS', 7: 'SALT', 8: 'LOSA', 9: 'SEAT',
             'NEWY': 1, 'CHIC': 2, 'WASH': 3, 'ATLA': 4, 'KANS': 5, 'HOUS': 6, 'SALT': 7, 'LOSA': 8, 'SEAT': 9}

# router numbers with an eBGP peer / session
has_eBGP = [1, 3, 5, 7, 8, 9] # NEWY, WASH, KANS, SALT, LOSA, SEAT

# key = router number
# list entries = interface name, interface IP address and OSPF weight (of links directly connected to the router)
link_dict = {1: [['chic', '.0.1.1/24', 1000], ['wash', '.0.4.1/24', 277], ['host', '.101.0.2/24', 0]],
             2: [['newy', '.0.1.2/24', 1000], ['wash', '.0.2.2/24', 905], ['atla', '.0.3.2/24', 1045], ['kans', '.0.6.1/24', 690], ['host', '.102.0.2/24', 0]],
             3: [['newy', '.0.4.2/24', 277], ['chic', '.0.2.1/24', 905], ['atla', '.0.5.1/24', 700], ['host', '.103.0.2/24', 0]],
             4: [['wash', '.0.5.2/24', 700], ['chic', '.0.3.1/24', 1045], ['hous', '.0.7.1/24', 1385], ['host', '.104.0.2/24', 0]],
             5: [['chic', '.0.6.2/24', 690], ['hous', '.0.8.1/24', 818], ['salt', '.0.9.1/24', 1330], ['host', '.105.0.2/24', 0]],
             6: [['atla', '.0.7.2/24', 1385], ['kans', '.0.8.2/24', 818], ['losa', '.0.10.1/24', 1705], ['host', '.106.0.2/24', 0], ['mgt', '.0.199.1/24', 0]],
             7: [['kans', '.0.9.2/24', 1330], ['losa', '.0.11.1/24', 1303], ['seat', '.0.12.1/24', 913], ['host', '.107.0.2/24', 0]],
             8: [['hous', '.0.10.2/24', 1705], ['salt', '.0.11.2/24', 1303], ['seat', '.0.13.1/24', 1342], ['host', '.108.0.2/24', 0]],
             9: [['losa', '.0.13.2/24', 1342], ['salt', '.0.12.2/24', 913], ['host', '.109.0.2/24', 0]]}


# configure router
def create_router_config(n_router):
    # enter configuration mode
    data_out.write('./go_to.sh '+name_dict[n_router]+'\n')
    data_out.write('vtysh\n')
    data_out.write('conf t\n')

    # configure all the interfaces (also corresponding OSPF weights)
    for link in link_dict[n_router]:
        data_out.write('interface '+link[0]+'\n')
        data_out.write('ip address '+str(nb)+link[1]+'\n')
        if link[2] != 0:
            data_out.write('ip ospf cost '+str(link[2])+'\n')
        data_out.write('exit\n')

    # configure all the iBGP sessions (and eBGP session)
    data_out.write('router bgp '+str(nb)+'\n')
    if n_router in has_eBGP:
        data_out.write('network '+str(nb)+'.0.0.0/8\n')
    for neighbor in [x for x in range(1, 10) if x != n_router]:
        data_out.write('neighbor '+str(nb)+'.10'+str(neighbor)+'.0.2 remote-as '+str(nb)+'\n')
        data_out.write('neighbor '+str(nb)+'.10'+str(neighbor)+'.0.2 update-source host\n')
        if n_router in has_eBGP:
            data_out.write('neighbor '+str(nb)+'.10'+str(neighbor)+'.0.2 next-hop-self\n')
    data_out.write('exit\n')

    # configure static route fro eBGP session
    if n_router in has_eBGP:
        data_out.write('ip route '+str(nb)+'.0.0.0/8 null0\n')

    # configure OSPF
    data_out.write('router ospf\n')
    for link in link_dict[n_router]:
        data_out.write('network '+str(nb)+link[1]+' area 0\n')
    data_out.write('exit\n')

    # back to home directory
    data_out.write('exit\n')
    data_out.write('exit\n')
    data_out.write('exit\n')


# configure interface and default route on host
def create_host_config(n_host):
    data_out.write('./go_to.sh '+name_dict[n_host]+'-host\n')
    data_out.write('ifconfig '+name_dict[n_host].lower()+' '+str(nb)+'.10'+str(n_host)+'.0.1/24\n')
    data_out.write('route add default gw '+str(nb)+'.10'+str(n_host)+'.0.2\n')
    data_out.write('exit\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser('Create Router Configuration')
    parser.add_argument('group_num', type=int, help='Group number')
    args = parser.parse_args()
    nb = args.group_num

    print 'Generate router configuration for group', nb

    with open('Config_AS_'+str(nb)+'.txt', 'w') as data_out:
        for i in range(1, 10):
            create_router_config(i)
            create_host_config(i)
            print 'Router/host config for', name_dict[i], 'generated!'

        data_out.write('\n')

    print '\n*** Copy & paste the content of file "'+'Config_AS_'+str(nb)+'.txt" into the home directory of the corresponding VM! ***\n'
