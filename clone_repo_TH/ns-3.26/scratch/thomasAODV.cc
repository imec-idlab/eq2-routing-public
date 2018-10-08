#include "ns3/qlrn-test-base.h"
#include <iostream>
#include <cmath>

// TODO https://www.nsnam.org/doxygen/manet-routing-compare_8cc_source.html

using namespace ns3;

/*
 * The idea is to create a grid of 100m by 100m, where a number of nodes (N) are randomly distributed,
 * the exact number of nodes can be specified via a parameter. One node is the Source (S), one is the
 * Destination (D) for a series of ICMP requests. The routing happens via AODV, all nodes are part of
 * the same MANET (ad hoc wifi network). Simulation parameters such as time between messages, nr
 * requests to send, timeout, duration of simulation can be specified via parameters. Output ideally
 * will be a graph showing the packet loss etc, so we can overlay other graphs to compare.
 *  _______________________________________________________________
 * |                                                               |
 * |  S                   N                  N             N       |
 * |            N                 N                   N            |
 * |                   N                    N                      |
 * |    N      N                 N                       N         |
 * |                  N                        N                   |
 * |                                         N           N         |
 * |      N        N           N                     N             |
 * |                                        N                      |
 * |          N            N                                       |
 * |                    N                  N       N         D     |
 * |_______________________________________________________________|
 */


int
main (int argc, char *argv[])
{
  //TODO :
    // https://groups.google.com/forum/#!topic/ns-3-users/2DUBAIRSA0k
    //
    // the actual differences are:
    // 1) SpectrumWifiPhy is slower.
    // 2) SpectrumWifiPhy allows to calculate the interference between Wi-Fi and other technologies using the same frequencies (e.g., LTE-LAA and 802.15.4).
    //
    // If you only have Wi-Fi, then YansWifiPhy is a good choice.

  //TODO look into this perhaps if issues with wifi interference exist?  https://groups.google.com/forum/#!topic/ns-3-users/h6roORws_oY

  //TODO : solve mystery of ARP not being replied to when given command
  //TODO : solve massive aodv traffic when a node moves out of position / back into position, should be left to Q to decide!?
  //TODO : loop tests
  //TODO : https://www.researchgate.net/post/How_to_vary_transmission_range_of_Wi-Fi_nodes_in_NS-3 -- try this for tests ?
  //TODO : what if non-best etsim has converged and other one hasnt ?
  //TODO : learn a lot from seeing repeated packets with lower TTL / ICMP TTL Exceeded
  //TODO : packets that are not able to get sent are dropped, fix that?
  //TODO : experiments with queue delay having an impact
  // ./waf --run "thomasAODV --doTest=test0.txt --eps=0.0 -learn=0.5 --traffic=ping --totalTime=38 --pcap"
  // (change runtime of pings)

  // LogComponentEnable ("QLearnerApplication", LOG_LEVEL_DEBUG);
  // LogComponentEnable ("AodvRoutingProtocol", LOG_LEVEL_DEBUG);
  // LogComponentEnable ("QTable", LOG_LEVEL_DEBUG);
  //
  // LogComponentEnable ("QLearnerApplication", LOG_LEVEL_ERROR);
  // LogComponentEnable ("AodvRoutingProtocol", LOG_LEVEL_ERROR);
  // LogComponentEnable ("QTable", LOG_LEVEL_ERROR);

  // LogComponentEnable ("AodvRoutingTable", LOG_LEVEL_DEBUG);
  // LogComponentEnable ("AodvRoutingTable", LOG_LEVEL_LOGIC);
  // LogComponentEnable ("ThomasAODV", LOG_LEVEL_ALL);
  // LogComponentEnable ("V4Ping", LOG_LEVEL_DEBUG);
  // LogComponentEnable ("Icmpv4L4Protocol", LOG_LEVEL_DEBUG);
  // LogComponentEnable ("Ipv4Interface", LOG_LEVEL_DEBUG);
  // LogComponentEnable ("ArpL3Protocol", LOG_LEVEL_DEBUG);

  // LogComponentEnable("Ipv4L3Protocol", LOG_LEVEL_ALL);
  // LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
  // LogComponentEnable ("UdpTraceClient", LOG_LEVEL_ALL);
  // LogComponentEnable ("UdpServer", LOG_LEVEL_ALL);
  // LogComponentEnable ("AodvNeighbors", LOG_LEVEL_DEBUG);

  /******************************************
  // TODO : a packet can be considered a lost packet if it exceeds a
  // specified delay threshold. p100 ITU-T spec trafficABC. dus packets en dan callback als ze aankomen met een bepaalde (hoge) delay ??
  ******************************************/
  Time::SetResolution (Time::NS);

  // ThomasAODV obj;
  QLearningBase obj;
  if ( !obj.Configure (argc, argv) )
    {
      NS_FATAL_ERROR ("Configuration failed. Aborted.");
    }
    Packet::EnablePrinting();

    // Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold",
    //                     StringValue ("2200"));


    //Not adding these lines causes a packet to be lost in test0 for seemingly no reason.
    // https://groups.google.com/forum/#!topic/ns-3-users/TiY9IUFnrZ4 <- seems like this bug perhaps, though it should have been fixed
    // Config::SetDefault ("ns3::ArpCache::AliveTimeout", TimeValue(Seconds (24*60*60)));
    // Config::SetDefault ("ns3::ArpCache::PendingQueueSize", UintegerValue (100));
  return obj.Run ();
}
