# include "ns3/aodv-module.h" /* aodv routing on nodes */
# include "ns3/mobility-module.h" /* makes STA mobile but we dont want any of that <-- needed for placement in grid */
# include "ns3/core-module.h" /* CommandLine (among others) */
# include "ns3/network-module.h" /* packets, sockets, links ? */
# include "ns3/internet-module.h" /* ipv4 / ipv6 / internet stack helper */
# include "ns3/point-to-point-module.h"    /* point to point = NIC / channel ctl */
# include "ns3/wifi-module.h" /* wifi stuff */
# include "ns3/v4ping-helper.h" /* Create a IPv4 ping application and associate it to a node. */

import ns.applications
import ns.core
import ns.internet
import ns.network
import ns.point_to_point
import ns.internet_apps
import ns.aodv
import ns.wifi
import ns.mobility
import thomasQLRN
import thomasQLRNHelper
import sys
import os

#
#  The idea is to create a grid of 100m by 100m, where a number of nodes (N) are randomly distributed,
#  the exact number of nodes can be specified via a parameter. One node is the Source (S), one is the
#  Destination (D) for a series of ICMP requests. The routing happens via AODV, all nodes are part of
#  the same MANET (ad hoc wifi network). Simulation parameters such as time between messages, nr
#  requests to send, timeout, duration of simulation can be specified via parameters. Output ideally
#  will be a graph showing the packet loss etc, so we can overlay other graphs to compare.
#   _______________________________________________________________
#  |                                                               |
#  |  S                   N                  N             N       |
#  |            N                 N                   N            |
#  |                   N                    N                      |
#  |    N      N                 N                       N         |
#  |                  N                        N                   |
#  |                                         N           N         |
#  |      N        N           N                     N             |
#  |                                        N                      |
#  |          N            N                                       |
#  |                    N                  N       N         D     |
#  |_______________________________________________________________|
#

# ns.core.LogComponentEnable("ThomasAODV", ns.core.LOG_LEVEL_INFO)

def GetPosition(node):
    #from wifi-ap.py
    mobility = node.GetObject(ns.mobility.MobilityModel.GetTypeId())
    return mobility.GetPosition()

class ThomasAODV:
    def __init__ (self):
        #   Number of nodes in the network (>= 2)
        self.numberOfNodes = 10
        #   Number of pings to exchange (>= 1)
        self.numberOfPings = 30
        #   Simulation time, seconds
        self.totalTime = 60.0
        #   Write per-device PCAP traces if true
        self.pcap = False
        #   Print routes if true
        self.printRoutes = False
        #   Link break somewhere?
        self.linkBreak = False
        self.node_to_break = 999999

        # Helpers
        self.mobility = ns.mobility.MobilityHelper()

        # testing stuff
        # self.aodv = thomasQLRN.QLRNER()
        # print(ns.aodv.__file__)
        # for i in dir(self.aodv):
        #     print(str(i) + "        :       " + str(i in dir(ns.aodv.AodvHelper) ) )
        self.aodv = ns.aodv.AodvHelper()

        # self.factory = ns.core.ObjectFactory()
        # self.factory.SetTypeId ("ns3::aodv::RoutingProtocol")
        #
        # self.b = self.factory.Create<"ns3::aodv::RoutingProtocol"> ()
        # self.c = self.factory.Create()
        # print("=")
        # print(self.b)
        # print(self.c)

        self.qlrn = thomasQLRNHelper.thomasQLRNHelper()
        self.tqLrn = thomasQLRN.thomasQLRN()
        print("a")
        open("aaaa.txt",'w').close()
        self.routingStream = ns.network.OutputStreamWrapper("aaaa.txt", os.O_WRONLY)
        print("b")
        self.tqLrn.Initialize()
        print("c")
        # print(self.tqLrn.IsInitialized())
        self.tqLrn.PrintRoutingTableAllAt (ns.core.Seconds (8), routingStream)


        # print(dir(self.tqLrn))
        # self.tqLrn.PrintRoutingTable(self.routingStream)
        print("c")



        #Network
        self.nodes = ns.network.NodeContainer()
        # self.devices = pointToPoint.Install(nodes)
        # self.interfaces = address.Assign(devices)
        self.devices = None
        self.interfaces = None

        # self.x = ns.internet_apps.V4PingHelper("10.0.0.0")
    def Configure (self, argv):
        #Random seed (how?) TODO

        #cmdline
        cmd = ns.core.CommandLine()

        cmd.numberOfNodes = None
        cmd.totalTime = None
        cmd.linkBreak = None
        cmd.printRoutes = None
        cmd.pcap = None

        cmd.NumNodesSide = None
        cmd.AddValue("","")
        cmd.AddValue ("pcap", "enable / disable pcap trace output")
        cmd.AddValue ("printRoutes", "enable / disable routing table output")
        cmd.AddValue ("numberOfNodes", "Number of nodes in the net, larger than 1")
        cmd.AddValue ("totalTime", "Simulation time in seconds")
        cmd.AddValue ("linkBreak", "Makes some node part of the path between src and dst unresponsive. inkBreak");

        cmd.Parse(argv)

        self.numberOfNodes = int(cmd.numberOfNodes) if cmd.numberOfNodes is not None else self.numberOfNodes
        self.totalTime = int(cmd.totalTime) if cmd.totalTime is not None else self.totalTime
        self.linkBreak = cmd.linkBreak if cmd.linkBreak is not None else self.linkBreak
        self.printRoutes = cmd.printRoutes if cmd.printRoutes is not None else self.printRoutes
        self.pcap = cmd.pcap if cmd.pcap is not None else self.pcap

        return True;

    def Run (self):
        # //  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
        self.CreateAndPlaceNodes ();
        self.CreateDevices ();
        self.InstallInternetStack ();
        self.InstallApplications ();

        print ( "Starting simulation for " + str( self.totalTime) + str( " s ...\n"))

        ns.core.Simulator.Stop(ns.core.Seconds (self.totalTime));
        ns.core.Simulator.Run ();
        ns.core.Simulator.Destroy ();


    def Report (self, output):
        # Ptr<OutputStreamWrapper> out = Create<OutputStreamWrapper> ("locations_after.txt", std::ios::out);
        # for (unsigned int i = 0; i < nodes.GetN (); i++) {
        # if (i != node_to_break)
        # {
        # *(out->GetStream ()) << nodes.Get(i)->GetObject<MobilityModel>()->GetPosition().x << "   "
        #   << nodes.Get(i)->GetObject<MobilityModel>()->GetPosition().y << "   "
        #   << ns3::Names::FindName (nodes.Get(i)) << std::endl;
        # }
        # }
        pass

    def CreateAndPlaceNodes(self):
        # creation
        self.nodes.Create (self.numberOfNodes)

        ns.core.Names.Add("ICMPsource", self.nodes.Get (0))
        for i in range(1,self.numberOfNodes-1):
            ns.core.Names.Add("node-"+str(i), self.nodes.Get (i))
        ns.core.Names.Add("ICMPdest", self.nodes.Get (self.numberOfNodes-1))

        self.mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

        x = ns.core.UniformRandomVariable()
        y = ns.core.UniformRandomVariable()
        x.SetAttribute ("Min", ns.core.DoubleValue(1.0));
        x.SetAttribute ("Max", ns.core.DoubleValue(1000.0));
        y.SetAttribute ("Min", ns.core.DoubleValue(1.0));
        y.SetAttribute ("Max", ns.core.DoubleValue(100.0));

        positionAlloc = ns.mobility.RandomRectanglePositionAllocator()
        positionAlloc.SetX(x)
        positionAlloc.SetY(y)
        self.mobility.SetPositionAllocator(positionAlloc)
        self.mobility.Install(self.nodes)

        self.mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                        "MinX", ns.core.DoubleValue(0.0),
						"MinY", ns.core.DoubleValue (0.0),
                        "LayoutType", ns.core.StringValue("RowFirst") )
        self.mobility.Install(self.nodes.Get(0))

        self.mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                        "MinX", ns.core.DoubleValue(1000.0),
						"MinY", ns.core.DoubleValue (100.0),
                        "LayoutType", ns.core.StringValue("RowFirst") )
        self.mobility.Install(self.nodes.Get(self.numberOfNodes-1))

        # if (linkBreak)
        # {
        # // Ptr<Node> node = nodes.Get (40);
        # Ptr<Node> node = nodes.Get (28);
        # Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
        # Simulator::Schedule (Seconds (20), &MobilityModel::SetPosition, mob, Vector (1e5, 1e5, 1e5));
        # }
        # self.PrintNodeLocs("locations_py.txt") #dont do this


    def CreateDevices(self):
        # Wifi : Default for now
        wifiMac = ns.wifi.WifiMacHelper()
        wifiMac.SetType ("ns3::AdhocWifiMac");

        channel = ns.wifi.YansWifiChannelHelper.Default()
        phy = ns.wifi.YansWifiPhyHelper.Default()
        # we create a channel object and associate it to our PHY layer object manager to make sure that all the PHY layer objects
        # created by the YansWifiPhyHelper share the same underlying channel, that is, they share the same wireless medium
        # and can communication and interfere:
        phy.SetChannel (channel.Create ())

        wifi = ns.wifi.WifiHelper()
        # Other possib : AARFWifiManager, for example
        wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",\
            ns.core.StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", ns.core.UintegerValue (0));
        self.devices = wifi.Install (phy, wifiMac, self.nodes);

        # if (pcap)
        # {
        #   phy.EnablePcapAll (std::string ("aodv"));
        # }
        # AsciiTraceHelper ascii;
        # phy.EnableAsciiAll (ascii.CreateFileStream ("my.tr"));


    def InstallInternetStack(self):
        # Perhaps later: IPv4List = multiple routing protocols in a priority order, first one to accept packet is chosen
        stack = ns.internet.InternetStackHelper();
        stack.SetRoutingHelper (self.aodv);
        stack.Install (self.nodes);

        addresses = ns.internet.Ipv4AddressHelper()
        addresses.SetBase(ns.network.Ipv4Address("10.1.1.0"), ns.network.Ipv4Mask("255.255.255.0"))

        self.interfaces = addresses.Assign (self.devices);
        if (self.printRoutes):
            fname1="aodv.routes"
            fname2="aodv2.routes"
            open(fname1, 'w').close()
            open(fname2, 'w').close()

            routingStream = ns.network.OutputStreamWrapper(fname1, os.O_WRONLY)
            routingStream2 = ns.network.OutputStreamWrapper(fname2, os.O_WRONLY)

            print(type(routingStream2))
            print(dir(routingStream2))

            #TODO         for flow_id, flow_stats in monitor.GetFlowStats(): (wifi olsr)
            # https://www.nsnam.org/doxygen/classns3_1_1_ipv4_routing_helper.html
            self.aodv.PrintRoutingTableAllAt (ns.core.Seconds (8), routingStream)
            self.aodv.PrintRoutingTableAllAt (ns.core.Seconds (48), routingStream2)

            # for (unsigned int i = 0; i < numberOfNodes ; i++)
            #   aodv.PrintRoutingTableAllAt (Seconds (8), routingStream);
            # for (unsigned int i = 0; i < numberOfNodes ; i++)
            #   aodv.PrintRoutingTableAllAt (Seconds (48), routingStream2);
        self.PrintNodeLocs("locations_py.txt")

    # remoteAddr = appSink.GetObject(ns.internet.Ipv4.GetTypeId()).GetAddress(1,0).GetLocal()

    def InstallApplications(self):
        # Ping (from aodv.cc)
        # Param : address that should be pinged
        ping = ns.internet_apps.V4PingHelper(self.interfaces.GetAddress (self.numberOfNodes-1))
        ping.SetAttribute ("Verbose", ns.core.BooleanValue (True))

        p = ping.Install(self.nodes.Get (0))
        # ApplicationContainer p = ping.Install (self.nodes.Get (0))
        p.Start (ns.core.Seconds (0))
        p.Stop (ns.core.Seconds (25) - ns.core.Seconds (0.001))
        if (self.linkBreak):
            q = ping.Install (self.nodes.Get (0))
            q.Start (ns.core.Seconds (30))
            q.Stop (ns.core.Seconds (198) - ns.core.Seconds (0.001))

    def PrintNodeLocs(self,outFile):
        #TODO linkbreak maybe dont print that one
        os = open(outFile, "w+")
        print(self.nodes.GetN())
        for i in xrange(self.nodes.GetN() ):
            location = GetPosition(self.nodes.Get(i))
            print >> os, str(location.x) + "   " \
                            + str(location.y) + "   " \
                            + str("...") + "   " \
                            + str(self.interfaces.GetAddress (i)) + "/" + ns.core.Names.FindName(self.nodes.Get(i));
        os.close()

if __name__ == "__main__":
    ns.network.Packet.EnablePrinting();
    obj = ThomasAODV()
    if ( not obj.Configure (sys.argv) ):
        NS_FATAL_ERROR ("Configuration failed. Aborted.");
    obj.Run ();
    obj.Report (ns.core.ofstream("output.txt")); #fine
  # // // PErhaps linkbreak function in TAODV object that can trigger a break ?
  # // // move node away
  # // Ptr<Node> node = nodes.Get (size/2);
  # // Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
  # // Simulator::Schedule (Seconds (totalTime/3), &MobilityModel::SetPosition, mob, Vector (1e5, 1e5, 1e5));

  # // // https://groups.google.com/forum/#!topic/ns-3-users/o_dh4wN-po0
  # // for (unsigned int i = 0; i < normalNodes.GetN (); i++) {
  # //   std::cout << normalNodes.Get(i)->GetObject<MobilityModel>()->GetPosition().x << "  " << normalNodes.Get(i)->GetObject<MobilityModel>()->GetPosition().y << std::endl;
  # // }
  #
  # // Simulator::Stop (Seconds (60) );
  # // Simulator::Run ();
  # // Simulator::Destroy ();

  # //TODO : find out what routing algorithm is being used currently
  # // because it sure as hell is not AODV
  # // https://groups.google.com/forum/#!topic/ns-3-users/A9dkt6xJ35Q
  # //earlier : link break = transfer node VERY far away
  # //  we wont do that
  #
  # // https://groups.google.com/forum/#!topic/ns-3-users/oL4fzgGi690
  # // aodv is slow as fu
#  return 0;
# }
