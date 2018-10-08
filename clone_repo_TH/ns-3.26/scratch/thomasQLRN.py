import ns.internet
import ns.core
import os

class thomasQLRN ( ns.internet.Ipv4RoutingProtocol ):
    def __init__(self):
        pass

    def Initialize(self):
        print("hi")

    def IsInitialized(self):
        return True

    def PrintRoutingTable(self, out):
        print >> out, str("ballentent") #+ "   "
                        # + str("...") + "   " \
                        # + str(self.interfaces.GetAddress (i)) + "/" + ns.core.Names.FindName(self.nodes.Get(i));

    def NotifyAddAddress (self, interface, address):
        pass


    def NotifyInterfaceDown (self, interface):
        pass


    def NotifyInterfaceUp (self, interface):
        pass


    def NotifyRemoveAddress (self, interface, address):
        pass


    def PrintRoutingTable (self, stream, unit=ns.core.Seconds) :
        print(type(stream))
        for i in dir(stream):
            if ("__" not in i):
                print(i)
        print >> stream, str("ballentent")


    # 	  Print the Routing Table entries. More...

    def RouteInput(self, p, header, idev, ucb, mcb, lcb, ecb):
        pass

    def RouteOutput(self, p, header, oif, sockerr):
        pass

    def SetIpv4(self, ipv4):
        pass
