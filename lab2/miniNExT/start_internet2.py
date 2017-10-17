#!/usr/bin/python

import sys
import os
import atexit
import argparse

# patch isShellBuiltin
import mininet.util
import mininext.util
mininet.util.isShellBuiltin = mininext.util.isShellBuiltin
sys.modules['mininet.util'] = mininet.util

from mininet.util import dumpNodeConnections
from mininet.node import OVSController, RemoteController
from mininet.log import setLogLevel, info

from mininext.cli import CLI
from mininext.net import MiniNExT
from mininet.link import Link, TCLink
from mininet.util import quietRun

from topo_internet2 import QuaggaInternet2Topo

# # Add path to Lab-1 files
# sys.path.append('/home/cs551/551-Labs/lab1/router')
# from lab3 import CS144Controller

from sets import Set

net = None

def addStaticNet(net):
    s1 = net.addHost( 'server1' )
    sr0 = net.addSwitch( 'sr0' )
    net.addLink( s1, sr0, intfName1='server1-eth0', intfName2='sr0-eth1' )
    net.addLink( sr0, net.getNodeByName('NEWY'), intfName1='sr0-eth2', intfName2='newy-sr0')
    net.addController('csr0', controller=RemoteController)
    
    s2 = net.addHost( 'server2' )
    net.addLink( s2, net.getNodeByName('SEAT'), intfName1='server2-eth0', intfName2='seat-server2' )

def setStaticNet(net, group_number):
    server1, server2, router = net.get( 'server1', 'server2', 'sr0' )
    s1intf = server1.defaultIntf()
    s1intf.setIP('192.168.2.2/24')
    s2intf = server2.defaultIntf()
    s2intf.setIP(str(group_number) + '.109.0.3/24')
    
    server1.cmd('route add %s/32 dev %s-eth0' % ('192.168.2.1', 'server1'))
    server1.cmd('route add default gw %s dev %s-eth0' % ('192.168.2.1', 'server1'))
    server1.cmd('route del -net %d.0.0.0/8 dev %s-eth0' % (group_number+2, 'server1'))

    server2.cmd('route add %s/32 dev %s-eth0' % (str(group_number) + '.109.0.4', 'server2'))
    server2.cmd('route add default gw %s dev %s-eth0' % (str(group_number) + '.109.0.4', 'server2'))
    server2.cmd('route del -net %d.0.0.0/8 dev %s-eth0' % (group_number, 'server2'))

    starthttp(server1)
    starthttp(server2)


def stopStaticNet(net):
    stophttp() 


def starthttp( host ):
    "Start simple Python web server on hosts"
    info( '*** Starting SimpleHTTPServer on host', host, '\n' )
    host.cmd( 'cd ../http_%s/; nohup python2.7 ./webserver.py &' % (host.name) )


def stophttp():
    "Stop simple Python web servers"
    info( '*** Shutting down stale SimpleHTTPServers', 
          quietRun( "pkill -9 -f SimpleHTTPServer" ), '\n' )    
    info( '*** Shutting down stale webservers', 
          quietRun( "pkill -9 -f webserver.py" ), '\n' )    


def startNetwork(topo_info, router_lo, init_quagga_configs, group_number):
    "instantiates a topo, then starts the network and prints debug information"

    info('** Creating Quagga network topology\n')
    topo = QuaggaInternet2Topo(topo_info, router_lo)

    global net
    #net = MiniNExT(topo, controller=OVSController)
    net = MiniNExT(topo, controller=RemoteController)

    info('** Adding Static networks\n')
    addStaticNet(net)

    info('** Starting the network\n')
    net.start()

    info('** Setting Static network\n')
    setStaticNet(net,group_number)

    info('** Adding links and loopback interfaces')
    links_set = Set()
    for router_name, ngh_list in topo_info.items():
            for ngh in ngh_list:

                # Make sure to only create one link between two routers
                if ngh+router_name not in links_set:
                    print 'Link added between '+str(router_name)+' and '+str(ngh)
                    Link(net.getNodeByName(router_name), net.getNodeByName(ngh), intfName1=ngh.lower(), intfName2=router_name.lower())
                    links_set.add(router_name+ngh)

            # Add link to the host
            Link(net.getNodeByName(router_name), net.getNodeByName(router_name+'-host'), intfName1='host', intfName2=router_name.lower())

    info('** Configuring the IP addresses **\n')
    for router_name in topo_info:
        node = net.getNodeByName(router_name)
        node.cmd('ifconfig ebgp_peer 0 up')
        #node.cmd('ifconfig lo '+router_lo[router_name]+'/24 up')
        for ngh, ip in topo_info[router_name].items():
            intf_name = ngh.lower()
            if init_quagga_configs:
                node.cmd('ifconfig '+intf_name+' 0.0.0.0/0 up')
            #else:
            #    node.cmd('ifconfig '+intf_name+' '+ip+' up')

    node = net.getNodeByName('HOUS')
    node.cmd('ifconfig mgt 0.0.0.0/0 up')

    info('** Dumping host connections\n')
    dumpNodeConnections(net.hosts)

    info('** Testing network connectivity\n')
    info('** Disabled **\n')
    #net.ping(net.hosts)

    info('** Dumping host processes\n')
    for host in net.hosts:
        if host.name in ['server1', 'server2']:
            continue
        host.cmdPrint("ps aux")

    info('** Running CLI\n')
    CLI(net)

    stopStaticNet(net)


def stopNetwork():
    "stops a network (only called on a forced cleanup)"

    if net is not None:
        info('** Tearing down Quagga network\n')
        net.stop()

def read_topo(infile, group_nb):
    "read the topology from a file. The topo is stored in a dictionnary"

    topo_info = {}
    router_lo = {}

    fd = open(infile, 'r')
    for line in fd.readlines():
        line = line.replace('_X_', str(group_nb))
        if line[0] != '#':
            linetab = line.split()

            # Get router id
            router_lo[linetab[0]] = linetab[3].rstrip()

            # Get topology
            if linetab[0] not in topo_info:
                topo_info[linetab[0]] = {}
            topo_info[linetab[0]][linetab[1]] = linetab[2]
    fd.close()

    return topo_info, router_lo

def write_quagga_configs(names, router_lo, group_number, dir_name):
    "write the quagga configs for all the routers in the topology, based on the skeleton"
    for name in names:

        if not os.path.exists(dir_name+'/'+name):
            os.makedirs(dir_name+'/'+name)

        for f in ['daemons', 'debian.conf', 'vtysh.conf', 'zebra.conf', 'ospfd.conf', 'bgpd.conf']:
            with open(dir_name+'/EXAMPLE/'+f) as fd:
                skelet = fd.read()
                skelet = skelet.replace('NAME', 'G'+str(group_number)+'_'+name)
                skelet = skelet.replace('127.0.0.1', router_lo[name])
            with open(dir_name+'/'+name+'/'+f, "w") as outfile:
                outfile.write(skelet)


if __name__ == '__main__':

    parser = argparse.ArgumentParser("Start the virtual topology.")
    parser.add_argument("group_nb", type=int, help="Group number.")
    parser.add_argument("infile", type=str, help="File where to find the IP addresses to use.")
    parser.add_argument('--new', action='store_true', help="If set, the quagga initial configs will be initialized to the default one")
    args = parser.parse_args()
    group_number = args.group_nb
    infile = args.infile
    init_quagga_configs = args.new

    # Read the topology
    topo_info, router_lo = read_topo(infile, group_number)

    # Initialize the Quagga config files
    if init_quagga_configs:
        write_quagga_configs(topo_info.keys(), router_lo, group_number, \
        '/root/configs')

    # Force cleanup on exit by registering a cleanup function
    # atexit.register(stopNetwork)

    # Tell mininet to print useful information
    setLogLevel('info')
    startNetwork(topo_info, router_lo, init_quagga_configs, group_number)

    info('** Quit miniNExT **')
