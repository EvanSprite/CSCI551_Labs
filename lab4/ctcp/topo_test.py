"""
Custom topology example
"""


from mininet.topo import Topo

""" Hw topo

   h1 --- s1 ---- h2
"""
class SimpleTopo( Topo ):
    def __init__(self):
        # Initialize topology
        Topo.__init__( self )

        # Add hosts and switches
        h1 = self.addHost( 'h1', inNamespace=False )
        h2 = self.addHost( 'h2', inNamespace=True )
        s1 = self.addSwitch( 's1' )

        # Add links
        self.addLink( h1, s1 )
        self.addLink( h2, s1 )


""" Line topo
Four linearly connected switches plus a host for each leftmost and rightmost switch:

   h1 --- sw1 --- sw2 --- sw3 --- sw4 --- h2
"""
class LineTopo( Topo ):
    def __init__( self ):
        # Initialize topology
        Topo.__init__( self )

        # Add hosts and switches
        h1 = self.addHost( 'h1', inNamespace=False )
        h2 = self.addHost( 'h2', inNamespace=True )
        s1 = self.addSwitch( 's1' )
        s2 = self.addSwitch( 's2' )
        s3 = self.addSwitch( 's3' )
        s4 = self.addSwitch( 's4' )

        # Add links
        self.addLink( h1, s1 )
        self.addLink( s1, s2 )
        self.addLink( s2, s3 )
        self.addLink( s3, s4 )
        self.addLink( s4, h2 )


""" Dumbbell topo
Two directly connected switches plus 3 hosts for each switch:

   host  \                     /  host    
          \                   /     
   host --- switch --- switch --- host
          /                   \
   host  /                     \  host
"""
class DumbbellTopo( Topo ):
    def __init__( self ):
        # Initialize topology
        Topo.__init__( self )

        # Add host
        leftHost1 = self.addHost( 'h1', inNamespace=False )
        leftHost2 = self.addHost( 'h3', inNamespace=False )
        leftHost3 = self.addHost( 'h5', inNamespace=False )
        rightHost1 = self.addHost( 'h2', inNamespace=True )
        rightHost2 = self.addHost( 'h4', inNamespace=True )
        rightHost3 = self.addHost( 'h6', inNamespace=True )
        leftSwitch = self.addSwitch( 's1' )
        rightSwitch = self.addSwitch( 's2' )

        # Add Links
        self.addLink( leftHost1, leftSwitch )
        self.addLink( leftHost2, leftSwitch )
        self.addLink( leftHost3, leftSwitch )
        self.addLink( leftSwitch, rightSwitch )
        self.addLink( rightSwitch, rightHost1 )
        self.addLink( rightSwitch, rightHost2 )
        self.addLink( rightSwitch, rightHost3 )

"""
Adding the 'topos' dict with a key/value pair to generate our newly defined
topology enables one to pass in '--topo=linetopo' from the command line.
"""
topos = { 'line': ( lambda: LineTopo() ),
          'dumbbell' : ( lambda: DumbbellTopo() ),
          'simple' : (lambda: SimpleTopo() ) }
