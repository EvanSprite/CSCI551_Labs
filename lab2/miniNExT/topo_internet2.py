"""
Example topology of Quagga routers
"""

import inspect
import os
from mininext.topo import Topo
from mininext.services.quagga import QuaggaService

net = None


class QuaggaInternet2Topo(Topo):

    "Creates a topology of Quagga routers, based on the Internet2 topology"

    def __init__(self, topo_info, router_lo):
        """Initialize a Quagga topology based on the topology information"""
        Topo.__init__(self)

        # Initialize a service helper for Quagga with default options
        quaggaSvc = QuaggaService(autoStop=False)

        # Path configurations for mounts
        quaggaBaseConfigPath = '/root/configs/'

        # Dictionnary used to store the hosts
        quagga_hosts = {}

        normal_hosts = {}

        # Setup each Quagga router
        for router_name in topo_info:
            # Create the quagga hosts
            quagga_hosts[router_name] = self.addHost(name=router_name,
                                        hostname=router_name,
                                        privateLogDir=True,
                                        privateRunDir=True,
                                        inMountNamespace=True,
                                        inPIDNamespace=True,
                                        inUTSNamespace=True)

            # Create the normal host connected to the quagga host above
            normal_hosts[router_name+'-host'] = self.addHost(name=router_name+'-host',
                                        hostname=router_name+'-host',
                                        privateLogDir=True,
                                        privateRunDir=True,
                                        inMountNamespace=True,
                                        inPIDNamespace=True,
                                        inUTSNamespace=True)

            # Add a loopback interface with an IP in router's announced range
            self.addNodeLoopbackIntf(router_name, ip=router_lo[router_name])

            # Configure and setup the Quagga service for this node
            quaggaSvcConfig = \
                {'quaggaConfigPath': quaggaBaseConfigPath + router_name}
            self.addNodeService(node=router_name, service=quaggaSvc,
                                nodeConfig=quaggaSvcConfig)

            print quaggaBaseConfigPath + router_name

        # Create an open v switch to connect the network to the ouside world
        ovs_switch = self.addSwitch('EBGP_PEER', dpid='1')

        # Connect all the routers to OVS, for potentiel future ebgp sessions
        for router_name in topo_info:
	    if router_name != 'HOUS':
                self.addLink(quagga_hosts[router_name], ovs_switch, intfName1=router_name.lower()+"-ovs", intfName2="ebgp_peer")

	# Add the management interface to HOUS
	self.addLink(quagga_hosts['HOUS'], ovs_switch, intfName1='hous-mgt-ovs', intfName2='mgt')

        # Setup each link between two Quagga routers
        """links_set = Set()
        i = 0
        for router_name, ngh_list in topo_info.items():
            for ngh_ip in ngh_list:
                ngh = ngh_ip[0]
                print 'Link added between '+str(router_name)+' and '+str(ngh)

                # Make sure to only create one link between two routers
                if ngh+router_name not in links_set:
                    self.addLink(quagga_hosts[router_name], quagga_hosts[ngh], \
                        intfName1=ngh.lower()+str(i), intfName2=router_name.lower()+str(i))
                    links_set.add(router_name+ngh)

            i += 0
        # List of Quagga host configs"""
        """quaggaHosts = []
        quaggaHosts.append(QuaggaHost(name='a1', ip='172.0.1.1/16',
                                      loIP='10.0.1.1/24'))
        quaggaHosts.append(QuaggaHost(name='b1', ip='172.0.2.1/16',
                                      loIP='10.0.2.1/24'))
        quaggaHosts.append(QuaggaHost(name='c1', ip='172.0.3.2/16',
                                      loIP='10.0.3.1/24'))
        quaggaHosts.append(QuaggaHost(name='c2', ip='172.0.3.1/16',
                                      loIP='10.0.3.1/24'))
        quaggaHosts.append(QuaggaHost(name='d1', ip='172.0.4.1/16',
                                      loIP='10.0.4.1/24'))
        quaggaHosts.append(QuaggaHost(name='rs', ip='172.0.254.254/16',
                                      loIP=None))

        # Add switch for IXP fabric
        ixpfabric = self.addSwitch('fabric-sw1')

        # Setup each Quagga router, add a link between it and the IXP fabric
        for host in quaggaHosts:

            # Create an instance of a host, called a quaggaContainer
            quaggaContainer = self.addHost(name=host.name,
                                           ip=host.ip,
                                           hostname=host.name,
                                           privateLogDir=True,
                                           privateRunDir=True,
                                           inMountNamespace=True,
                                           inPIDNamespace=True,
                                           inUTSNamespace=True)

            # Add a loopback interface with an IP in router's announced range
            self.addNodeLoopbackIntf(node=host.name, ip=host.loIP)

            # Configure and setup the Quagga service for this node
            quaggaSvcConfig = \
                {'quaggaConfigPath': quaggaBaseConfigPath + host.name}
            self.addNodeService(node=host.name, service=quaggaSvc,
                                nodeConfig=quaggaSvcConfig)

            # Attach the quaggaContainer to the IXP Fabric Switch
            self.addLink(quaggaContainer, ixpfabric)"""
