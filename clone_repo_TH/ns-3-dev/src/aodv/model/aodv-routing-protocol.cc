/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Based on
 *      NS-2 AODV model developed by the CMU/MONARCH group and optimized and
 *      tuned by Samir Das and Mahesh Marina, University of Cincinnati;
 *
 *      AODV-UU implementation by Erik Nordström of Uppsala University
 *      http://core.it.uu.se/core/index.php/AODV-UU
 *
 * Authors: Elena Buchatskaia <borovkovaes@iitp.ru>
 *          Pavel Boyko <boyko@iitp.ru>
 */
#define NS_LOG_APPEND_CONTEXT                                   \
  //if (m_ipv4) { std::clog << "[AODV node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; }

#include "aodv-routing-protocol.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/random-variable-stream.h"
#include "ns3/inet-socket-address.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/udp-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/icmpv4.h"
#include "ns3/pointer.h"
#include <algorithm>
#include <limits>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("AodvRoutingProtocol");

namespace aodv
{
NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

/// UDP Port for AODV control traffic
const uint32_t RoutingProtocol::AODV_PORT = 654;

//-----------------------------------------------------------------------------
/// Tag used by AODV implementation

class DeferredRouteOutputTag : public Tag
{

public:
  DeferredRouteOutputTag (int32_t o = -1) : Tag (), m_oif (o) {}

  static TypeId GetTypeId ()
  {
    static TypeId tid = TypeId ("ns3::aodv::DeferredRouteOutputTag")
      .SetParent<Tag> ()
      .SetGroupName("Aodv")
      .AddConstructor<DeferredRouteOutputTag> ()
    ;
    return tid;
  }

  TypeId  GetInstanceTypeId () const
  {
    return GetTypeId ();
  }

  int32_t GetInterface() const
  {
    return m_oif;
  }

  void SetInterface(int32_t oif)
  {
    m_oif = oif;
  }

  uint32_t GetSerializedSize () const
  {
    return sizeof(int32_t);
  }

  void  Serialize (TagBuffer i) const
  {
    i.WriteU32 (m_oif);
  }

  void  Deserialize (TagBuffer i)
  {
    m_oif = i.ReadU32 ();
  }

  void  Print (std::ostream &os) const
  {
    os << "DeferredRouteOutputTag: output interface = " << m_oif;
  }

private:
  /// Positive if output device is fixed in RouteOutput
  int32_t m_oif;
};

NS_OBJECT_ENSURE_REGISTERED (DeferredRouteOutputTag);


//-----------------------------------------------------------------------------
RoutingProtocol::RoutingProtocol () :
  m_nr_of_lrn_dropped(0),
  m_qlearner(0),
  m_qlrn_packets_sent(0),
  m_control_packets_sent(0),
  m_traffic_packets_sent(0),
  m_learning_packets_sent(0),
  m_other_packets_sent(0),
  m_qlrn_packets_received(0),
  m_control_packets_received(0),
  m_traffic_packets_received(0),
  m_learning_packets_received(0),
  m_other_packets_received(0),
  m_rreqRetries (2),
  m_ttlStart (1),
  m_ttlIncrement (2),
  m_ttlThreshold (7),
  m_timeoutBuffer (2),
  m_rreqRateLimit (10),
  m_rerrRateLimit (10),
  m_activeRouteTimeout (Seconds (3)),
  m_netDiameter (35),
  m_nodeTraversalTime (MilliSeconds (40)),
  m_netTraversalTime (Time ((2 * m_netDiameter) * m_nodeTraversalTime)),
  m_pathDiscoveryTime ( Time (2 * m_netTraversalTime)),
  m_myRouteTimeout (Time (2 * std::max (m_pathDiscoveryTime, m_activeRouteTimeout))),
  m_helloInterval (Seconds (1)),
  m_allowedHelloLoss (2),
  m_deletePeriod (Time (5 * std::max (m_activeRouteTimeout, m_helloInterval))),
  m_nextHopWait (m_nodeTraversalTime + MilliSeconds (10)),
  m_blackListTimeout (Time (m_rreqRetries * m_netTraversalTime)),
  m_maxQueueLen (64),
  m_maxQueueTime (Seconds (30)),
  m_destinationOnly (false),
  m_gratuitousReply (true),
  m_enableHello (false),
  m_routingTable (m_deletePeriod),
  m_queue (m_maxQueueLen, m_maxQueueTime),
  m_requestId (0),
  m_seqNo (0),
  m_rreqIdCache (m_pathDiscoveryTime),
  m_dpd (m_pathDiscoveryTime),
  m_nb (m_helloInterval),
  m_rreqCount (0),
  m_rerrCount (0),
  m_htimer (Timer::CANCEL_ON_DESTROY),
  m_rreqRateLimitTimer (Timer::CANCEL_ON_DESTROY),
  m_rerrRateLimitTimer (Timer::CANCEL_ON_DESTROY),
  m_lastBcastTime (Seconds (0))
{
  m_nb.SetCallback (MakeCallback (&RoutingProtocol::SendRerrWhenBreaksLinkToNextHop, this));
  m_output_filestream = 0;
  m_traffic_destinations = std::vector<Ipv4Address>();
  m_running_avg_latency = std::make_pair<int,float>(0,0);
  m_prev_delay = 0;
  m_prev_delay_per_prev_hop = std::map<Ipv4Address,uint64_t>();
  m_traffic_packets_received_per_src = std::map<Ipv4Address,unsigned int>();
  // m_src_aodvProto = std::map<Ipv4Address,Ptr<aodv::RoutingProtocol> >();
}

TypeId
RoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::aodv::RoutingProtocol")
    .SetParent<Ipv4RoutingProtocol> ()
    .SetGroupName("Aodv")
    .AddConstructor<RoutingProtocol> ()
    .AddAttribute ("HelloInterval", "HELLO messages emission interval.",
                   TimeValue (Seconds (1)),
                   MakeTimeAccessor (&RoutingProtocol::m_helloInterval),
                   MakeTimeChecker ())
    .AddAttribute ("TtlStart", "Initial TTL value for RREQ.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&RoutingProtocol::m_ttlStart),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("TtlIncrement", "TTL increment for each attempt using the expanding ring search for RREQ dissemination.",
                   UintegerValue (2),
                   MakeUintegerAccessor (&RoutingProtocol::m_ttlIncrement),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("TtlThreshold", "Maximum TTL value for expanding ring search, TTL = NetDiameter is used beyond this value.",
                   UintegerValue (7),
                   MakeUintegerAccessor (&RoutingProtocol::m_ttlThreshold),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("TimeoutBuffer", "Provide a buffer for the timeout.",
                   UintegerValue (2),
                   MakeUintegerAccessor (&RoutingProtocol::m_timeoutBuffer),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("RreqRetries", "Maximum number of retransmissions of RREQ to discover a route",
                   UintegerValue (2),
                   MakeUintegerAccessor (&RoutingProtocol::m_rreqRetries),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("RreqRateLimit", "Maximum number of RREQ per second.",
                   UintegerValue (10),
                   MakeUintegerAccessor (&RoutingProtocol::m_rreqRateLimit),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("RerrRateLimit", "Maximum number of RERR per second.",
                   UintegerValue (10),
                   MakeUintegerAccessor (&RoutingProtocol::m_rerrRateLimit),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("NodeTraversalTime", "Conservative estimate of the average one hop traversal time for packets and should include "
                   "queuing delays, interrupt processing times and transfer times.",
                   TimeValue (MilliSeconds (40)),
                   MakeTimeAccessor (&RoutingProtocol::m_nodeTraversalTime),
                   MakeTimeChecker ())
    .AddAttribute ("NextHopWait", "Period of our waiting for the neighbour's RREP_ACK = 10 ms + NodeTraversalTime",
                   TimeValue (MilliSeconds (50)),
                   MakeTimeAccessor (&RoutingProtocol::m_nextHopWait),
                   MakeTimeChecker ())
    .AddAttribute ("ActiveRouteTimeout", "Period of time during which the route is considered to be valid",
                   TimeValue (Seconds (3)),
                   MakeTimeAccessor (&RoutingProtocol::m_activeRouteTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("MyRouteTimeout", "Value of lifetime field in RREP generating by this node = 2 * max(ActiveRouteTimeout, PathDiscoveryTime)",
                   TimeValue (Seconds (11.2)),
                   MakeTimeAccessor (&RoutingProtocol::m_myRouteTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("BlackListTimeout", "Time for which the node is put into the blacklist = RreqRetries * NetTraversalTime",
                   TimeValue (Seconds (5.6)),
                   MakeTimeAccessor (&RoutingProtocol::m_blackListTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("DeletePeriod", "DeletePeriod is intended to provide an upper bound on the time for which an upstream node A "
                   "can have a neighbor B as an active next hop for destination D, while B has invalidated the route to D."
                   " = 5 * max (HelloInterval, ActiveRouteTimeout)",
                   TimeValue (Seconds (15)),
                   MakeTimeAccessor (&RoutingProtocol::m_deletePeriod),
                   MakeTimeChecker ())
    .AddAttribute ("NetDiameter", "Net diameter measures the maximum possible number of hops between two nodes in the network",
                   UintegerValue (35),
                   MakeUintegerAccessor (&RoutingProtocol::m_netDiameter),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("NetTraversalTime", "Estimate of the average net traversal time = 2 * NodeTraversalTime * NetDiameter",
                   TimeValue (Seconds (2.8)),
                   MakeTimeAccessor (&RoutingProtocol::m_netTraversalTime),
                   MakeTimeChecker ())
    .AddAttribute ("PathDiscoveryTime", "Estimate of maximum time needed to find route in network = 2 * NetTraversalTime",
                   TimeValue (Seconds (5.6)),
                   MakeTimeAccessor (&RoutingProtocol::m_pathDiscoveryTime),
                   MakeTimeChecker ())
    .AddAttribute ("MaxQueueLen", "Maximum number of packets that we allow a routing protocol to buffer.",
                   UintegerValue (64),
                   MakeUintegerAccessor (&RoutingProtocol::SetMaxQueueLen,
                                         &RoutingProtocol::GetMaxQueueLen),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxQueueTime", "Maximum time packets can be queued (in seconds)",
                   TimeValue (Seconds (30)),
                   MakeTimeAccessor (&RoutingProtocol::SetMaxQueueTime,
                                     &RoutingProtocol::GetMaxQueueTime),
                   MakeTimeChecker ())
    .AddAttribute ("AllowedHelloLoss", "Number of hello messages which may be loss for valid link.",
                   UintegerValue (2),
                   MakeUintegerAccessor (&RoutingProtocol::m_allowedHelloLoss),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("GratuitousReply", "Indicates whether a gratuitous RREP should be unicast to the node originated route discovery.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&RoutingProtocol::SetGratuitousReplyFlag,
                                        &RoutingProtocol::GetGratuitousReplyFlag),
                   MakeBooleanChecker ())
    .AddAttribute ("DestinationOnly", "Indicates only the destination may respond to this RREQ.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RoutingProtocol::SetDestinationOnlyFlag,
                                        &RoutingProtocol::GetDestinationOnlyFlag),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableHello", "Indicates whether a hello messages enable.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&RoutingProtocol::SetHelloEnable,
                                        &RoutingProtocol::GetHelloEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableBroadcast", "Indicates whether a broadcast data packets forwarding enable.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&RoutingProtocol::SetBroadcastEnable,
                                        &RoutingProtocol::GetBroadcastEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("UniformRv",
                   "Access to the underlying UniformRandomVariable",
                   StringValue ("ns3::UniformRandomVariable"),
                   MakePointerAccessor (&RoutingProtocol::m_uniformRandomVariable),
                   MakePointerChecker<UniformRandomVariable> ())
    .AddAttribute ("OutputData",
                   "Access to the underlying m_output_data_to_file",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RoutingProtocol::m_output_data_to_file),
                   MakeBooleanChecker())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                   MakeTraceSourceAccessor (&RoutingProtocol::m_txTrace),
                   "ns3::Packet::TracedCallback")
    .AddTraceSource ("Rx", "A new packet is created and is sent",
                   MakeTraceSourceAccessor (&RoutingProtocol::m_rxTrace),
                   "ns3::Packet::TracedCallback")

  ;
  return tid;
}

bool RoutingProtocol::IamAmongTheDestinations() {
  return std::find(m_traffic_destinations.begin(), m_traffic_destinations.end(), m_ipv4->GetAddress(1,0).GetLocal() ) != m_traffic_destinations.end();
}

void
RoutingProtocol::SetMaxQueueLen (uint32_t len)
{
  m_maxQueueLen = len;
  m_queue.SetMaxQueueLen (len);
}
void
RoutingProtocol::SetMaxQueueTime (Time t)
{
  m_maxQueueTime = t;
  m_queue.SetQueueTimeout (t);
}

RoutingProtocol::~RoutingProtocol ()
{
}


/* taken from QLearner in case of no_q */
void RoutingProtocol::UpdateAvgDelay(PacketTimeSentTag ptst_tag, PortNrTag pnt) {
  if (pnt.GetLearningPkt() ) {
    // only interested in actual traffic.. what does learning traffic delay matter ?
  } else {
    m_running_avg_latency.first += 1;
    m_running_avg_latency.second =
      (
        (
          m_running_avg_latency.second * ( m_running_avg_latency.first-1)
        )
        + (Simulator::Now() - ptst_tag.GetSentTime()).GetInteger()
      )
      / m_running_avg_latency.first;
       std::cout << "updateavgdelay --> delay of this packet: "<<(Simulator::Now() - ptst_tag.GetSentTime()).GetSeconds() << std::endl;
      // std::cout << m_running_avg_latency.second << std::endl;
  }
}

/* taken from QLearner in case of no_q */
void RoutingProtocol::OutputDataToFile(PacketTimeSentTag ptst_tag, Ptr<const Packet> p, bool learning_packet, TrafficType t, Ipv4Address sourceIP) {
  // p->Print(std::cout);std::cout<<std::endl;
  PortNrTag pnt;
  NS_ASSERT(p->PeekPacketTag(pnt) || CheckAODVHeader(p));

  uint64_t currDelay = (Simulator::Now() - ptst_tag.GetSentTime()).GetInteger();

  m_prev_delay = (m_prev_delay_per_prev_hop[ptst_tag.GetPrevHop()] == 0? currDelay:m_prev_delay_per_prev_hop[ptst_tag.GetPrevHop()]);
  m_prev_delay_per_prev_hop[ptst_tag.GetPrevHop()] = currDelay;

  uint64_t jitter_val = ( currDelay < m_prev_delay ? m_prev_delay - currDelay : currDelay - m_prev_delay );
  float total_traffic_recv_by_me = float(RecvTrafficStatistics(sourceIP) + (pnt.GetLearningPkt() ? 1 : 0));
  float total_traffic_sent_to_me = float(pnt.GetPktNumber() + (!pnt.GetLearningPkt() ? 1 : 0)) /*because 1/0 is undefined, we add one (pkt number is 0-index)*/ ;
  float packet_loss = 1 - total_traffic_recv_by_me / (pnt.GetLearningPkt() ? total_traffic_recv_by_me : total_traffic_sent_to_me);

  bool delay_ok = currDelay < TrafficTypeReqsMap[t].GetDelayMax();
  bool jitter_ok = jitter_val < TrafficTypeReqsMap[t].GetJitterMax();
  bool packet_loss_ok = packet_loss< TrafficTypeReqsMap[t].GetRandomLossMax();

  NS_ASSERT_MSG(IamAmongTheDestinations(), "Not an intended destination, so why is it outputting as if it is ? " << m_ipv4->GetAddress(1,0).GetLocal());
  std::cout << "output to file --> delay of this packet: "<<(Simulator::Now() - ptst_tag.GetSentTime()).GetSeconds() << std::endl;
  std::cout << "output to file --> delay of this packet: "<<(Simulator::Now() - ptst_tag.GetSentTime()).As(Time::MS) << std::endl;


  *(m_output_filestream->GetStream()) << p->GetUid() << "," << Simulator::Now().As(Time::S) << ","
          << (Simulator::Now() - ptst_tag.GetSentTime()).As(Time::MS) << "," << ptst_tag.GetInitialEstim();
  if (learning_packet)  { *(m_output_filestream->GetStream()) << ",learning,OK,OK,OK"; }
  else                  {
    *(m_output_filestream->GetStream()) << ",not_learning"
        << (delay_ok?",OK":",NOK")
        << (jitter_ok?",OK":",NOK")
        << (packet_loss_ok?",OK":",NOK");
  }
  // else                  { *(m_output_filestream->GetStream()) << ",not_learning"<< (delay_ok&&jitter_ok&&packet_loss_ok?",OK":",NOK"); }
  *(m_output_filestream->GetStream()) << "," << ptst_tag.GetPrevHop();
  *(m_output_filestream->GetStream()) << "," << traffic_type_to_traffic_string(t) << "\n";

  (m_output_filestream->GetStream())->flush();
}


void
RoutingProtocol::DoDispose ()
{
  m_ipv4 = 0;
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::iterator iter =
         m_socketAddresses.begin (); iter != m_socketAddresses.end (); iter++)
    {
      iter->first->Close ();
    }
  m_socketAddresses.clear ();
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::iterator iter =
         m_socketSubnetBroadcastAddresses.begin (); iter != m_socketSubnetBroadcastAddresses.end (); iter++)
    {
      iter->first->Close ();
    }
  m_socketSubnetBroadcastAddresses.clear ();
  Ipv4RoutingProtocol::DoDispose ();
}

void
RoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
  *stream->GetStream () << "Node: " << m_ipv4->GetObject<Node> ()->GetId ()
                        << "; Time: " << Now().As (unit)
                        << ", Local time: " << GetObject<Node> ()->GetLocalTime ().As (unit)
                        << ", AODV Routing table" << std::endl;

  m_routingTable.Print (stream);
  *stream->GetStream () << std::endl;
}

int64_t
RoutingProtocol::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_uniformRandomVariable->SetStream (stream);
  return 1;
}

bool RoutingProtocol::CheckAODVHeader(Ptr<const Packet> p) {
  /**
  * another workaround for tests ...
  * this time, due to SeqTsHeader being seen as a AODV header
  * weird (OPIED FROM QLEARNER CheckAODVHeader)
  */
  if (m_qlearner && p->GetSize() == TEST_UDP_NO_ECHO_PKT_SIZE) {
    return false;
  }

  // Could be that there is still an UDPHeader surrounding the packet,
  // so we must check that case seperately...

  Ptr<Packet> p_copy = p->Copy();
  aodv::TypeHeader tHeader (aodv::AODVTYPE_RREP_ACK);
  UdpHeader u;

  p_copy->PeekHeader (tHeader);
  if (tHeader.IsValid()) {
    return true;
  } else {
    p_copy->RemoveHeader(u);
    p_copy->RemoveHeader (tHeader);
    return tHeader.IsValid();
  }
}

bool RoutingProtocol::CheckIcmpTTLExceeded(Ptr<const Packet> p, Icmpv4TimeExceeded& ii) {
  // so the first impl does not catch icmp ttl exceeded as they are sent, second one (ret_st2) does. so most interesting is if ret st1  finds one that ret st2
  // does not, then we should get an error

  Ptr<Packet> p_copy = p->Copy();
  Icmpv4Header i;
  Ipv4Header ipv4header;
  bool ret_st2 = false;

  if (p->GetSize() == 36) {
    p_copy->RemoveHeader(i);
    if ( i.GetType() == 11) {
      p_copy->RemoveHeader(ii);
      ipv4header = ii.GetHeader();
      ret_st2 = (ipv4header.GetTtl() == 0) && !(ipv4header.GetDestination() == ipv4header.GetSource() && ipv4header.GetSource() == Ipv4Address("102.102.102.102"));
    } else {
      ret_st2 = false;
    }
  }
  else if (p->GetSize() == 32 || p->GetSize() == 64) {
    p_copy->RemoveHeader(ii);
    ipv4header = ii.GetHeader();
    ret_st2 = (ipv4header.GetTtl() == 0) && !(ipv4header.GetDestination() == ipv4header.GetSource() && ipv4header.GetSource() == Ipv4Address("102.102.102.102"));
  }
  else {
    ret_st2 = false;
  }
  return ret_st2;
}

void RoutingProtocol::CheckTraffic(Ptr<const Packet> p, TrafficType& t) {
  Icmpv4Header i;
  Icmpv4TimeExceeded ii;
  UdpHeader u;
  PortNrTag pnt;

  p->PeekHeader(i);
  p->PeekHeader(u);
  p->PeekPacketTag(pnt);

  if ((i.GetType() == 0 || i.GetType() == 3 || i.GetType() == 5 || i.GetType() == 8 || i.GetType() == 11) && pnt.GetDstPort() == 0)   {
    p->PeekHeader(ii);
    auto ipv4header = ii.GetHeader();
    if (ipv4header.GetTtl() == 0 && !(ipv4header.GetDestination() == ipv4header.GetSource() && ipv4header.GetSource() == Ipv4Address("102.102.102.102"))) { //added abt the ip for failed peekHeader
      t = OTHER; //TTL exceeded
    } else if (p->GetSize() == 64 /* echo */ || p->GetSize() == 36 /* TTL exceeded WITH ICMP HEADER ATTACHED*/) {
      if (CheckIcmpTTLExceeded(p,ii)) {
        t = OTHER;
      } else {
        // Very ugly workaround to "properly" detect ICMP, some AODV packets were being seen as ICMP unfortunately :(
          //(this is originally talking about the size check, then its grown from there.)
        t = ICMP;
      }
    } else if (p->GetSize() == TEST_UDP_ECHO_PKT_SIZE) {
      //used only in test cases...
      // UDP ECHO goes on port 9998, but at dst when he sends there is no udp header so we dont know the port and it looks like ICMP traffic
      // so we make an exception here to set the type correctly, so the test runs.
      // In production we dont intend to use UDP ECHO traffic so i this feels acceptable ...
      t = UDP_ECHO;
    }
  }
  else if ((u.GetDestinationPort() == 0 || u.GetSourcePort() == 0) && p->GetSize() != TEST_UDP_ECHO_PKT_SIZE && pnt.GetDstPort() == 0) { }
  else if (u.GetDestinationPort() == 9998 || u.GetSourcePort() == 9998 || p->GetSize() == TEST_UDP_ECHO_PKT_SIZE || pnt.GetDstPort() == 9998) { t = UDP_ECHO; } //used only in test cases, UDP-echo traffic on this port
  else if (u.GetDestinationPort() == 9999 || u.GetSourcePort() == 9999 || pnt.GetDstPort() == 9999) { t = WEB; }
  else if (u.GetDestinationPort() == 10000 || u.GetSourcePort() == 10000 || pnt.GetDstPort() == 10000) { t = VOIP; }
  else if (u.GetDestinationPort() == 10001 || u.GetSourcePort() == 10001 || pnt.GetDstPort() == 10001) { t = VIDEO; }
  else if (u.GetDestinationPort() == PORT_NUMBER_TRAFFIC_A || u.GetSourcePort() == PORT_NUMBER_TRAFFIC_A || pnt.GetDstPort() == PORT_NUMBER_TRAFFIC_A) { t = TRAFFIC_A; }
  else if (u.GetDestinationPort() == PORT_NUMBER_TRAFFIC_A+10 || u.GetSourcePort() == PORT_NUMBER_TRAFFIC_A+10 || pnt.GetDstPort() == PORT_NUMBER_TRAFFIC_A+10) { t = TRAFFIC_A; }
  else if (u.GetDestinationPort() == PORT_NUMBER_TRAFFIC_B || u.GetSourcePort() == PORT_NUMBER_TRAFFIC_B || pnt.GetDstPort() == PORT_NUMBER_TRAFFIC_B) { t = TRAFFIC_B; }
  else if (u.GetDestinationPort() == PORT_NUMBER_TRAFFIC_B+10 || u.GetSourcePort() == PORT_NUMBER_TRAFFIC_B+10 || pnt.GetDstPort() == PORT_NUMBER_TRAFFIC_B+10) { t = TRAFFIC_B; }
  else if (u.GetDestinationPort() == PORT_NUMBER_TRAFFIC_C || u.GetSourcePort() == PORT_NUMBER_TRAFFIC_C || pnt.GetDstPort() == PORT_NUMBER_TRAFFIC_C) { t = TRAFFIC_C; }
  else if (u.GetDestinationPort() == PORT_NUMBER_TRAFFIC_C+10 || u.GetSourcePort() == PORT_NUMBER_TRAFFIC_C+10 || pnt.GetDstPort() == PORT_NUMBER_TRAFFIC_C+10) { t = TRAFFIC_C; }
  else if ( i.GetType() == 4 || i.GetType() == 9 || i.GetType() == 10 || i.GetType() == 11
              || i.GetType() == 12 || i.GetType() == 13 || i.GetType() == 14 || i.GetType() == 15
              || i.GetType() == 16 || i.GetType() == 18 || i.GetType() == 17 || i.GetType() == 130) {
    NS_LOG_ERROR("confused packet::");
    std::stringstream ss; p->Print(ss); NS_LOG_ERROR(ss.str());
    NS_ASSERT_MSG(false, "weird icmp");
  } else {
    //Will be printed in the else { } below
    // p->Print(std::cout);std::cout<<std::endl;
    // std::cout << t << std::endl;
    NS_ASSERT(t == OTHER);
  }
  // p->Print(std::cout<<std::endl);std::cout<< "   ttype:" << traffic_type_to_traffic_string(t) <<std::endl;
  return;
}

void RoutingProtocol::CorrectPacketTrackingOutput(Ptr<const Packet> p, Ipv4Header header) {
  QLrnHeader q;
  QoSQLrnHeader qosq;

  PortNrTag pnt;
  TrafficType t = OTHER;

  CheckTraffic(p, t);
  p->PeekPacketTag(pnt);
  if ( t == ICMP || t == VIDEO || t == WEB || t == VOIP || t == UDP_ECHO || t == TRAFFIC_A || t == TRAFFIC_B || t == TRAFFIC_C ) {
    if (pnt.GetLearningPkt()) {
      m_learning_packets_sent -= 1;
    } else {
      m_traffic_packets_sent -= 1;
    }
  } else if (m_qlearner && (m_qlearner->CheckQLrnHeader(p,q) || m_qlearner->CheckQLrnHeader(p,qosq))){ NS_ASSERT_MSG(false, "Ideally this would only happen for traffic packets.(1)");
  } else if (CheckAODVHeader(p)) { NS_ASSERT_MSG(false, "Ideally this would only happen for traffic packets (2)).");
  } else if (m_qlearner == 0) {
    return;
  } else {
    if (m_qlearner && header.GetSource() == m_ipv4->GetAddress(1,0).GetLocal())  {
      std::cout << "Unknown packet on the output routing (i, node" << m_qlearner->GetNode()->GetId() << " dont know how to count this)" << std::endl;
      p->Print(std::cout);std::cout<<std::endl<<std::endl;
      // NS_ASSERT(m_qlearner->GetNode()->GetId() == 0);
      // NS_ASSERT(false);
    } else {
      // header source is set to 102.102.102.102 (i.e. not initialised ??)
      // so this packet must be counted at the L2 level, since it's the udp header we need, and thats where we can be sure it will have one ...
      // std::cout << "this is the packet were having trouble with. At node : " << m_ipv4->GetObject<Node>()->GetId() << std::endl;
      // std::cout << "hm ?"  << header.GetSource() << std::endl;
      // p->Print(std::cout);std::cout<<std::endl<<std::endl;
      // so i suppose leave this, do nothing..

      // legacy , due to pnt fix this isnt per se needed (it is for non-OnOffAppl traffic)
    }
  }
}

void RoutingProtocol::PacketTrackingOutput(Ptr<const Packet> p, Ipv4Header header) {
  QLrnHeader q;
  QoSQLrnHeader qosq;
  PortNrTag pnt;
  TrafficType t = OTHER;

  CheckTraffic(p, t);

  bool HasQLrnHeader = false;
  if (m_qlearner) { HasQLrnHeader = (m_qlearner->CheckQLrnHeader(p,q) || m_qlearner->CheckQLrnHeader(p,qosq)); }

  if ( t == ICMP || t == VIDEO || t == WEB || t == VOIP || t == UDP_ECHO || t == TRAFFIC_A || t == TRAFFIC_B || t == TRAFFIC_C ) {
    p->PeekPacketTag(pnt);
    if (pnt.GetLearningPkt()){ m_learning_packets_sent += 1; }
    else { m_traffic_packets_sent += 1; /* p->Print(std::cout<<std::endl);*/  }
  } else if (HasQLrnHeader) {
    m_qlrn_packets_sent += 1;
  } else if (CheckAODVHeader(p)) {
    // p->Print(std::cout << "parsed as AODV" << std::endl);std::cout<<std::endl;
    m_control_packets_sent += 1;
  } else if (m_qlearner == 0) {
    return;
  } else {
    if (m_qlearner && header.GetSource() == m_ipv4->GetAddress(1,0).GetLocal())  {
      std::cout << "Unknown packet on the output routing (i, node" << m_qlearner->GetNode()->GetId() << " dont know how to count this)" << std::endl;
      p->Print(std::cout);std::cout<<std::endl<<std::endl;
      // NS_ASSERT(m_qlearner->GetNode()->GetId() == 0);
      // NS_ASSERT(false);
    } else {
      // header source is set to 102.102.102.102 (i.e. not initialised ??)
      // so this packet must be counted at the L2 level, since it's the udp header we need, and thats where we can be sure it will have one ...
      // std::cout << "this is the packet were having trouble with. At node : " << m_ipv4->GetObject<Node>()->GetId() << std::endl;
      // std::cout << "hm ?"  << header.GetSource() << std::endl;
      // p->Print(std::cout);std::cout<<std::endl<<std::endl;
      // so i suppose leave this, do nothing..

      // legacy , due to pnt fix this isnt per se needed (it is for non-OnOffAppl traffic)
    }
  }
}
void RoutingProtocol::PacketTrackingInput(Ptr<const Packet> p, Ipv4Header header) {
  // packet reaches us
  TrafficType t = OTHER;
  PortNrTag pnt;
  QLrnInfoTag tagg; //part of method2

  p->PeekPacketTag(pnt);

  CheckTraffic(p,t);

  if ( (   t == ICMP      || t == VIDEO     || t == WEB     || t == VOIP || t == UDP_ECHO
        || t == TRAFFIC_A || t == TRAFFIC_B || t == TRAFFIC_C )) {
    // if its a traffic packet _and_ if we are not the sender (cause they might get sent back / Deferred Route)
    // if (m_ipv4->GetAddress(1,0).GetLocal() == Ipv4Address("10.1.1.4")) {
    // packets being forwarded are counted as traffic_rec in intermediate nodes
    //   std::cout << m_traffic_packets_received << std::endl;

    //Deferred output now also passes here, so we dont count that as we have already counted it in the regular transmission fct
    DeferredRouteOutputTag tag; //or we will be off-by-one for every deferred output
    std::stringstream ss; p->Print(ss);

   /*
    * 2 methods shown below, first uses PNT markings to decide if a packet is learning traffic or not whiel second method looks only at QLrnInfoTag.
    * If the QLrnInfoTag is missing, its not learning, if its there, its learning traffic.
    *
    * Difference becomes relevant in the case of dropped packets: ICMP TTL Exceeded means non-learning in the first case while it means learning in the second case
    * since TTL Exceeded on a route is a very important indicator for how good / bad a route is, I believe it should be considered learning and thus, method 2 seems
    * most appropriate to me. Either way, only difference is a small number of packets being counted differently.
    */

    /* METHOD 1*/
    if (pnt.GetLearningPkt() && !p->PeekPacketTag (tag)) { m_learning_packets_received++; }
    else if (!p->PeekPacketTag (tag)) { m_traffic_packets_received++; m_traffic_packets_received_per_src[header.GetSource()]++; }

    /* METHOD 2*/
    // if (p->PeekPacketTag (tagg) && !p->PeekPacketTag (tag)) { m_learning_packets_received++; }
    // else if (!p->PeekPacketTag (tagg)) { m_traffic_packets_received++; }
  }
}

std::string RoutingProtocol::GetQStatistics() {
  std::stringstream ss;
  ss << QStatistics();
  return ss.str();
}
std::string RoutingProtocol::GetAODVStatistics() {
  std::stringstream ss;
  ss << AODVStatistics();
  return ss.str();
}
std::string RoutingProtocol::GetTrafficStatistics() {
  std::stringstream ss;
  ss << TrafficStatistics();
  return ss.str();
}
std::string RoutingProtocol::GetLrnTrafficStatistics() {
  std::stringstream ss;
  ss << LrnTrafficStatistics();
  return ss.str();
}
std::string RoutingProtocol::GetRecvTrafficStatistics() {
  std::stringstream ss;
  ss << RecvTrafficStatistics();
  return ss.str();
}
std::string RoutingProtocol::GetRecvLrnTrafficStatistics() {
  std::stringstream ss;
  ss << RecvLrnTrafficStatistics();
  return ss.str();
}

void
RoutingProtocol::Start ()
{
  // TraceConnectWithoutContext ("Tx", MakeCallback(&RoutingProtocol::Packet TrackingOutput , this));
  // TraceConnectWithoutContext ("Rx", MakeCallback(&RoutingProtocol::Packet TrackingInput , this  ));

  if (m_output_data_to_file && IamAmongTheDestinations() ) {
    std::stringstream ss; ss << "out_stats_aodv_node"<<m_ipv4->GetObject<Node> ()->GetId ()<<".csv";
    m_output_filestream = Create<OutputStreamWrapper> (ss.str(), std::ios::out);
    *(m_output_filestream->GetStream ()) << "pktID,currTime,delay,initial_estim,learning,metric_delay,metric_jitter,metric_loss,second_to_last_hop,trafficType\n";
  }

  NS_LOG_FUNCTION (this);
  if (m_enableHello)
    {
      m_nb.ScheduleTimer ();
    }
  m_rreqRateLimitTimer.SetFunction (&RoutingProtocol::RreqRateLimitTimerExpire,
                                    this);
  m_rreqRateLimitTimer.Schedule (Seconds (1));

  m_rerrRateLimitTimer.SetFunction (&RoutingProtocol::RerrRateLimitTimerExpire,
                                    this);
  m_rerrRateLimitTimer.Schedule (Seconds (1));

}

Ptr<Ipv4Route>
RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header,
                              Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
  TrafficType t = OTHER;
  //We are the source of packet
  // m_txTrace(p);
  PacketTrackingOutput(p, header);
  CheckTraffic(p,t);
  PacketTimeSentTag ptst_tag;
  if (m_qlearner) {
    m_qlearner->HandleRouteOutput(p, header, t);
  } else if (!m_qlearner && !(p->PeekPacketTag(ptst_tag) ) ) {
    ptst_tag.SetPrevHop(m_ipv4->GetAddress(1,0).GetLocal());
    ptst_tag.SetSentTime(Simulator::Now().GetInteger());
    p->AddPacketTag(ptst_tag);
  }

  if (!p)
    {
      NS_LOG_DEBUG("Packet is == 0");
      return LoopbackRoute (header, oif); // later
    }
  if (m_socketAddresses.empty ())
    {
      sockerr = Socket::ERROR_NOROUTETOHOST;
      NS_LOG_LOGIC ("No aodv interfaces");
      Ptr<Ipv4Route> route;
      return route;
    }
  sockerr = Socket::ERROR_NOTERROR;
  Ptr<Ipv4Route> route;
  Ipv4Address dst = header.GetDestination ();

  RoutingTableEntry rt;
  if (m_routingTable.LookupValidRoute (dst, rt))
    {
      // ->RouteOutput
      // .RouteOutput
      route = rt.GetRoute ();
      Ptr<Ipv4Route> route_copy = Create<Ipv4Route>(*route); // make a copy here so we dont overwrite the entry in aodv rtable
                                                       // while still changing the route being used
      // route_copy->SetDestination (route->GetDestination());
      // route_copy->SetGateway (route->GetGateway());
      // route_copy->SetOutputDevice (route->GetOutputDevice());
      // route_copy->SetSource (route->GetSource());

      if (m_qlearner) {
        // (moved up to not avoid the checks etc)
        if (!m_qlearner->Route(route_copy, p, header)) { // checked for Delay-Change-Route-Issue
          CorrectPacketTrackingOutput(p, header);
          //Drop it, no neighbours = no routes...
          Ptr<Ipv4Route> route;
          return route;
        }
      }

      NS_ASSERT (route_copy != 0);
      NS_LOG_DEBUG ("Exist route to " << route_copy->GetDestination () << " from interface " << route_copy->GetSource () << " " << Simulator::Now().As(Time::S));
      if (oif != 0 && route_copy->GetOutputDevice () != oif)
        {
          NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
          sockerr = Socket::ERROR_NOROUTETOHOST;
          return Ptr<Ipv4Route> ();
        }
      UpdateRouteLifeTime (dst, m_activeRouteTimeout);
      UpdateRouteLifeTime (route->GetGateway (), m_activeRouteTimeout);
      NS_LOG_DEBUG("" << *route_copy << " " << p->GetUid());
      return route_copy;
    }

  // Valid route not found, in this case we return loopback.
  // Actual route request will be deferred until packet will be fully formed,
  // routed to loopback, received from loopback and passed to RouteInput (see below)
  uint32_t iif = (oif ? m_ipv4->GetInterfaceForDevice (oif) : -1);
  DeferredRouteOutputTag tag (iif);
  PortNrTag pnt;

  NS_LOG_DEBUG ("Valid Route not found, sending it to LoopBackRoute with a DeferredOutputTag  pktid:" << p->GetUid()) ;
  p->PeekPacketTag(pnt);
  if (!p->PeekPacketTag (tag)) {
      if (pnt.GetLearningPkt() ){
        // drop it  / code taken from no aodv interfaces / dont waste time enqueueing it.
        // -- Ptr<Ipv4Route> route;
        // -- return route;
        // this first attempt makes no traffic flow at all, because all learning traffic is immediately stopped
        // instead : (only drop if there is already a pakcet in the queue for the Dst, drop it)
        if (m_queue.Find(header.GetDestination() ) ) {
          CorrectPacketTrackingOutput(p,header); // we already counted the packet before, but now we're dropping it
          Ptr<Ipv4Route> route;
          return route;
        } else {  }
      }
      p->AddPacketTag (tag);
    }
  return LoopbackRoute (header, oif);
}

void
RoutingProtocol::DeferredRouteOutput (Ptr<const Packet> p, const Ipv4Header & header,
                                      UnicastForwardCallback ucb, ErrorCallback ecb)
{
  NS_LOG_FUNCTION (this << p << header);
  NS_ASSERT (p != 0 && p != Ptr<Packet> ());

  if (header.GetDestination() == m_ipv4->GetAddress(1,0).GetLocal()) {
      // std::cout << "dropping packet because its deferred and the destination is ourselves..?" << std::endl;
      std::stringstream ss; p->Print(ss);
      NS_LOG_DEBUG("Dropping a packet because it was deferred output and the destination is ourselves.\n" << ss.str());
      return;
  }

  QueueEntry newEntry (p, header, ucb, ecb);
  bool result = m_queue.Enqueue (newEntry); //<deze>
  if (result)
    {
      NS_LOG_DEBUG ("Add packet " << p->GetUid () << " to queue. Protocol " << (uint16_t) header.GetProtocol ());
      RoutingTableEntry rt;
      bool result = m_routingTable.LookupRoute (header.GetDestination (), rt);

      /**
       * I added a check here, because if there is already a VALID route present, we also should not send another RREQ!
       * that caused a lot of problems, where a packet arrived, no route was present, was sent to LoopBackRoute
       * then a new RREP arrived, for a RREQ we sent previously, that made the route valid
       *
       * the packet we sent to loopback then arrived, and caused another RREQ to be sent  which invalidates
       * the route we had just added, causing the packet to be queued waiting for the RREP. Since this happens with some regularity
       * : a QLRN packet that was sent in response to a RREP gets queued, RREQ gets sent, RREP response arrives, QLRN packet wants
       * to be sent for the RREP, gets queued since RREP not processed, etc...
       * so this check was added.
       */
          /**
           * A new issue arose, where the AODV kept flip-flopping between two equal choices, I'm not sure why it does that and frankly I dont care
           * I've simply disabled the invalidating when QLearning takes over, this makes sense anyway. Now running tests to see if anything
           * due to that
           */
      if (m_qlearner) {
        //New check
        if(!result || (rt.GetFlag () != IN_SEARCH && result && rt.GetFlag() != VALID ) ) {
          NS_LOG_DEBUG ("Send new RREQ for outbound packet to " <<header.GetDestination ());
          SendRequest (header.GetDestination ());
        }
      } else {
        //Old check (preserve behaviour of AODV for --no_q trials)
        if(!result || ((rt.GetFlag () != IN_SEARCH) && result ) ) {
          NS_LOG_DEBUG ("Send new RREQ for outbound packet to " <<header.GetDestination ());
          SendRequest (header.GetDestination ());
        }
      }
    }
}

bool
RoutingProtocol::RouteInput (Ptr<const Packet> _p, const Ipv4Header &header,
                             Ptr<const NetDevice> idev, UnicastForwardCallback ucb,
                             MulticastForwardCallback mcb, LocalDeliverCallback lcb, ErrorCallback ecb)
{
  // We are not the source of the packet (unless deferred)
  // https://groups.google.com/forum/#!msg/ns-3-users/BpZSAEVJwzI/AO2sTQRIGQ8J
  PacketTrackingInput(_p, header);
  Ptr<Packet> p = _p->Copy();

  bool randomDecidedDuplicate = false;

  NS_LOG_FUNCTION (this << p->GetUid () << header.GetDestination () << idev->GetAddress ());

  DeferredRouteOutputTag tag;
  DropPacketTag dpt;
  QLrnInfoTag tagg;
  QRoutedTrafficPacketTag q_rt_pkt_tag = QRoutedTrafficPacketTag(false);
  PacketTimeSentTag ptst_tag;
  PortNrTag pnt;

  QLrnHeader q;
  TrafficType t = OTHER;
  CheckTraffic(p, t);
  Ipv4Address dst = header.GetDestination ();
  Ipv4Address origin = header.GetSource ();

  if (m_qlearner) {
    m_qlearner->HandleRouteInput(p, header, p->PeekPacketTag(tag), randomDecidedDuplicate, t);
    if (p->PeekPacketTag(dpt)){
      if ( dpt.GetDrop() ) {
        std::stringstream ss; p->Print(ss << std::endl);
        NS_LOG_DEBUG("Dropping a pkt " << ss.str());
        return true; //drop the learning packet that isnt needed anymore bc upstream is converged
      }
    }
  } else if (p->PeekPacketTag(ptst_tag) && p->PeekPacketTag(pnt) && IamAmongTheDestinations() ){
    // the IamAmongTheDestinations in the if can be removed, then all nodes keep a notion of avg delay...
    // std::stringstream ss; p->Print(ss<<std::endl); NS_LOG_UNCOND("node" << m_ipv4->GetObject<Node>()->GetId() << "\n" << ss.str());
    // std::cout <<(p->PeekPacketTag(ptst_tag) ? " has a ptst_tag": "does not have a ptst tag") << "\n";
    // std::cout <<(p->PeekPacketTag(pnt) ? " has a pnt": "does not have a pnt") << "\n";
    UpdateAvgDelay(ptst_tag, pnt);
  }
  if (m_socketAddresses.empty ())
    {
      NS_LOG_LOGIC ("No aodv interfaces");
      return false;
    }

  NS_ASSERT (m_ipv4 != 0);
  NS_ASSERT (p != 0);
  // Check if input device supports IP
  NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
  int32_t iif = m_ipv4->GetInterfaceForDevice (idev);

  // Deferred route request
  if (idev == m_lo)
    {
	  DeferredRouteOutputTag tag;
      if (p->PeekPacketTag (tag))
        {
          DeferredRouteOutput (p, header, ucb, ecb);
          return true;
        }
    }

  // Duplicate of own packet if only AODV is going on
  if (!m_qlearner && IsMyOwnAddress (origin)) {
    NS_LOG_DEBUG ("AODV-dropping a packet b/c we sent it and also received it. pktId:" << p->GetUid());
    return true;
  } else if (m_qlearner && IsMyOwnAddress (origin)) {
    // So basically this part is what causes some packet loss in ICMP traffic
    // If we're QLearning, we must allow some room for error, thus dont automatically drop duplicates please
    if (randomDecidedDuplicate) {
      NS_LOG_DEBUG ("QLRN-not dropping a packet b/c we sent it, then received it back with random. pktId:" << p->GetUid());
    } else if (p->PeekPacketTag(tagg)) {
      NS_LOG_DEBUG ("QLRN-not dropping a packet b/c we sent it, then received it but QTag is there, bad decision?? UPDATE OF Q ??. pktId:" << p->GetUid());
      NS_LOG_DEBUG ("IE MAYBE SET A BOOL OR SO TO MAKE A HIGHER ESTIMATE GO BACK TO NEXT HOP");
    } else if (p->PeekPacketTag(q_rt_pkt_tag)) {
      if (q_rt_pkt_tag.PacketIsQRouted()) { } // fine, dont drop
      else {
        NS_LOG_UNCOND ("Dropping a packet.");
        return true; // drop it like it's hot
       }
    } else {
      // only redeeming quality for this case is in case its a node just sending interference traffic, then we will allow it
      // TODO add a real condition here... - if we decide to work with interferers again
      if (true) {
        NS_LOG_UNCOND ("Dropping a packet.");
        //drop it anyways :)
        return true;
      }
      NS_LOG_DEBUG("QLRN-for real dropping a packet, we got it back, its not Q, its not random. pktId:" << p->GetUid());

      // I dont really want this to happen so stop sim if it does
      p->Print(std::cout);std::cout<<" Dropping! pktid: "<<p->GetUid()<< "  (do we need to change Q values a bit to indicate this?)"<< dst<<std::endl;

      NS_ASSERT_MSG(false, "Error: we saw a packet that we sent ourselves. We are node " << m_qlearner->GetNode()->GetId() << ". The packet was " << p->GetUid() << ".");

      return true;
    }
  }

  // AODV is not a multicast routing protocol
  if (dst.IsMulticast ())
    {
      return false;
    }

  // Broadcast local delivery/forwarding
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ipv4InterfaceAddress iface = j->second;
      if (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()) == iif)
        if (dst == iface.GetBroadcast () || dst.IsBroadcast ())
          {
            if (m_dpd.IsDuplicate (p, header))
              {
                NS_LOG_DEBUG ("Duplicated packet " << p->GetUid () << " from " << origin << ". Drop.");
                return true;
              }
            UpdateRouteLifeTime (origin, m_activeRouteTimeout);
            Ptr<Packet> packet = p->Copy ();
            if (lcb.IsNull () == false)
              {
                NS_LOG_LOGIC ("Broadcast local delivery to " << iface.GetLocal ());
                lcb (p, header, iif);
                // Fall through to additional processing
              }
            else
              {
                NS_LOG_ERROR ("Unable to deliver packet locally due to null callback " << p->GetUid () << " from " << origin);
                ecb (p, header, Socket::ERROR_NOROUTETOHOST);
              }
            if (!m_enableBroadcast)
              {
                return true;
              }
            if (header.GetProtocol () == UdpL4Protocol::PROT_NUMBER)
              {
                UdpHeader udpHeader;
                p->PeekHeader (udpHeader);
                if (udpHeader.GetDestinationPort () == AODV_PORT)
                  {
                    // AODV packets sent in broadcast are already managed
                    return true;
                  }
              }
            if (header.GetTtl () > 1)
              {
                NS_LOG_LOGIC ("Forward broadcast. TTL " << (uint16_t) header.GetTtl ());
                NS_LOG_DEBUG ("header has TTL > 1, and not AODV, and bcast");
                RoutingTableEntry toBroadcast;
                if (m_routingTable.LookupRoute (dst, toBroadcast))
                  {
                    Ptr<Ipv4Route> route = toBroadcast.GetRoute ();
                    //Probably best not to change the broadcast addressing? Since it should (by def) be all the 1-hop neighbours + we do unicast
                    ucb (route, packet, header);
                  }
                else
                  {
                    NS_LOG_DEBUG ("No route to forward broadcast. Drop packet " << p->GetUid ());
                  }
              }
            else
              {
                NS_LOG_DEBUG ("TTL exceeded. Drop packet " << p->GetUid ());
              }
            return true;
          }
    }

  // Unicast local delivery
  if (m_ipv4->IsDestinationAddress (dst, iif))
    {
      UpdateRouteLifeTime (origin, m_activeRouteTimeout);
      RoutingTableEntry toOrigin;
      if (m_routingTable.LookupValidRoute (origin, toOrigin))
        {
          UpdateRouteLifeTime (toOrigin.GetNextHop (), m_activeRouteTimeout);

          m_nb.Update (toOrigin.GetNextHop (), m_activeRouteTimeout);
        }
      if (lcb.IsNull () == false)
        {
          NS_LOG_DEBUG ("Unicast local delivery to destination " << dst << "  packet id=" << p->GetUid());
          /**
            * So because RouteInput happens first, and then this is called, we already got rid of the PTST Tag
            * in case of a packet that should indeed be locally delivered.
            */
          lcb (p, header, iif);

          if (!m_qlearner && m_output_data_to_file) {
            PacketTimeSentTag ptst_tag; bool has_ptst_tag = p->PeekPacketTag(ptst_tag);
            bool is_aodv_traffic = CheckAODVHeader(p);
            if (!has_ptst_tag && !is_aodv_traffic) {
              NS_FATAL_ERROR("should at least be either AODV or have a tag");
            } else if (!has_ptst_tag && is_aodv_traffic) {
            } else if (has_ptst_tag && !is_aodv_traffic) {
              OutputDataToFile(ptst_tag, p, CheckAODVHeader(p), t, header.GetSource());
            } else { //aodv traffic WITH already a tag present, suspect
              OutputDataToFile(ptst_tag, p, CheckAODVHeader(p), t, header.GetSource());
              // NS_FATAL_ERROR("this also shouldnt happen");
            }
          }
        }
      else
        {
          NS_LOG_ERROR ("Unable to deliver packet locally due to null callback " << p->GetUid () << " from " << origin);
          ecb (p, header, Socket::ERROR_NOROUTETOHOST);
        }
      return true;
    }

  // Check if input device supports IP forwarding
  if (m_ipv4->IsForwarding (iif) == false)
    {
      NS_LOG_LOGIC ("Forwarding disabled for this interface");
      ecb (p, header, Socket::ERROR_NOROUTETOHOST);
      return true;
    }

  if (!m_qlearner) {
    PacketTimeSentTag ptst_tag;
    NS_ASSERT(p->PeekPacketTag(ptst_tag));
    p->RemovePacketTag(ptst_tag);
    ptst_tag.SetPrevHop(m_ipv4->GetAddress(1,0).GetLocal());
    p->AddPacketTag(ptst_tag);
  }

  // Forwarding
  return Forwarding (p, header, ucb, ecb);
}

bool
RoutingProtocol::Forwarding (Ptr<const Packet> _p, const Ipv4Header & header,
                             UnicastForwardCallback ucb, ErrorCallback ecb)
{
  NS_LOG_FUNCTION (this);
  Ptr<Packet> p = ConstCast<Packet> (_p);
  Ipv4Address dst = header.GetDestination ();
  Ipv4Address origin = header.GetSource ();
  m_routingTable.Purge (); // this also causes issues
  RoutingTableEntry toDst;

  if (m_qlearner) {
    if (!m_qlearner->CheckDestinationKnown(dst) ) {
      p->Print(std::cout);std::cout<<"  "<<p->GetUid()<<std::endl;
    }
    NS_ASSERT_MSG(m_qlearner->CheckDestinationKnown(dst), "Destination " << dst << " was not known for QLearner in forwarding function at node "<< m_qlearner->GetNode()->GetId() <<".");
    QLrnHeader q;
    QoSQLrnHeader qosq;
    if (m_qlearner->CheckQLrnHeader(p,q) || m_qlearner->CheckQLrnHeader(p, qosq)) {
      NS_LOG_DEBUG("We received a QLRN packet not destined for us and tried to forward it, most likely we received it by accident. Drop." );
      return true;
    }
  }

  if (m_routingTable.LookupRoute (dst, toDst)) {
      NS_LOG_DEBUG("A route is indeed present for " << dst );
      NS_LOG_DEBUG( *(toDst.GetRoute()) << ". Flag is " << toDst.GetFlag() << " (0VALID,2SEARCH,1INVALID). Route lifetime: "<< toDst.GetLifeTime().As(Time::S));
      NS_LOG_DEBUG("Interface to use: " << toDst.GetInterface() );
      if (toDst.GetFlag () == VALID)
        {
          Ptr<Ipv4Route> route = toDst.GetRoute ();
          Ptr<Ipv4Route> route_to_use = Create<Ipv4Route>(*route);


          if (m_qlearner) {
            if (!m_qlearner->Route(route_to_use, p, header)) { // checked for Delay-Change-Route-Issue
              // the reason this is now commented is because QLrnInfo packets are also dropped if they see that all their possible neighbours are converged values.
              // In that case, the agent only explores 100*eps % of the time and that can be any random node
              // NS_FATAL_ERROR("I dont think this actually ever happens because if we have to forward a packet we should indeed also have a neighbour to use.");
              //Drop it, no neighbours = no routes...
              std::stringstream ss; p->Print(ss<<std::endl<<p->GetUid()<< " ");
              Icmpv4TimeExceeded ii;
              PortNrTag pnt; p->PeekPacketTag(pnt);
              NS_ASSERT_MSG(
                ((p->GetSize( ) == 180 || p->GetSize() == 308) && pnt.GetLearningPkt()) ||
                 CheckIcmpTTLExceeded(p, ii)
                 , "[AODV node " << m_ipv4->GetObject<Node> ()->GetId () << "] " << "this should only be learning traffic right? otherwise (if no neighbours) we shouldnt get this far!" << ss.str());
              // this is wrong actually I think ! only has a point if we're not the source of the packet & the intermediate nodes can decide to drop packets
              m_nr_of_lrn_dropped += 1; // to verify wireshark pkt count with lrn_rec + lrn_sent at node0
              return true;
            }
          }

          /*
           *  Each time a route is used to forward a data packet, its Active Route
           *  Lifetime field of the source, destination and the next hop on the
           *  path to the destination is updated to be no less than the current
           *  time plus ActiveRouteTimeout.
           */
          UpdateRouteLifeTime (origin, m_activeRouteTimeout);
          UpdateRouteLifeTime (dst, m_activeRouteTimeout);
          UpdateRouteLifeTime (route->GetGateway (), m_activeRouteTimeout);

          UpdateRouteLifeTime (route_to_use->GetGateway (), m_activeRouteTimeout);
          // added route to use because of error being noticed where changes in route did not persist
          /*
           *  Since the route between each originator and destination pair is expected to be symmetric, the
           *  Active Route Lifetime for the previous hop, along the reverse path back to the IP source, is also updated
           *  to be no less than the current time plus ActiveRouteTimeout
           */
          RoutingTableEntry toOrigin;
          m_routingTable.LookupRoute (origin, toOrigin);

          // Possibly do this and update _that_ route life time, but underlying AODV stuff should not matter actually...
          // Ptr<Ipv4Route> tmp = Create<Ipv4Route>(*(toOrigin.GetRoute()));

          UpdateRouteLifeTime (toOrigin.GetNextHop (), m_activeRouteTimeout);

          m_nb.Update (route->GetGateway (), m_activeRouteTimeout);
          m_nb.Update (route_to_use->GetGateway (), m_activeRouteTimeout); // added route to use because of error being noticed where changes in route did not persist

          if (origin == m_ipv4->GetAddress(1,0).GetLocal()) {
            // Dont update, as there is no route to myself ???
          } else {
            m_nb.Update (toOrigin.GetNextHop (), m_activeRouteTimeout);

          }

          ucb (route_to_use, p, header);
          // route->SetGateway(old_route.GetGateway()); //possible fix to issue of QLearner permanently overwriting routes --
          // so this was the problem , route was being set back to what it was before, if we delay a bit this is immediately noticed
          // using route_to_use should fix it ?
          return true;
        }
      else if (!m_qlearner)
        {
          if (toDst.GetValidSeqNo ())
            {
              SendRerrWhenNoRouteToForward (dst, toDst.GetSeqNo (), origin);
              NS_LOG_DEBUG ("Drop packet " << p->GetUid () << " because no route to forward it (case no_qlrn).");
              return false;
            }
        }
        /*
          this part is important :
          if the aodv does not know a route, it will send a RERR back when we reach this part
          but thats not allowed by us, so we set a qlrn route and solve it that way
        */
      else if (m_qlearner)
        {
          //The route we found is not valid, but there _is_ a route
          // --> Simply take the object and apply the QRoute change and send it


          Ptr<Ipv4Route> route = Create<Ipv4Route>(*(toDst.GetRoute ()));
          if (!m_qlearner->Route(route, p, header)) { // checked for Delay-Change-Route-Issue
            // checking pkt forwarding counting too many
            Icmpv4TimeExceeded ii;
            if (!CheckIcmpTTLExceeded(p,ii)) {
              NS_FATAL_ERROR("I dont think this actually ever happens because if we have to forward a packet we should indeed also have a neighbour to use. Seperate case if its ICMP TTL Exc, drop those.");
            }
            //Drop it, no neighbours = no routes...
            return true;
          }
          // NS_ASSERT_MSG(toDst.GetRoute()->GetGateway() != Ipv4Address("102.102.102.102"), "Hm so I guess we should have fixRoute here anyway." ); // -- same
          m_qlearner->FixRoute(route, m_ipv4->GetObject<Ipv4L3Protocol> ()->GetNetDevice (1) , origin); // -- this shouldnt actually be necessary -- turns out it is, ADD_DST_CASE_7

          NS_LOG_DEBUG( "Tried to forward packet " << p->GetUid() << " for " << header.GetDestination() << " via " << toDst.GetRoute ()->GetGateway() << ".");
          NS_LOG_DEBUG( "Instead, we're going to route it via " << route->GetGateway());
          // NS_LOG_DEBUG( *(toDst.GetRoute()) << "   " << toDst.GetFlag() << " (0 is valid_d_)    " << toDst.GetLifeTime().As(Time::S) << "  " << toDst.GetInterface() );
          ucb (route, p, header);

          return true;
        }
  }
  if (m_qlearner) {
    // So this is the case when the AODV does not have a route _at all_ for the given packet
    // Thus need to "create" an Ipv4Route object according to what we believe the route should be
    // And then try to send the packet with that route

    if (toDst.GetRoute()->GetGateway() == Ipv4Address("102.102.102.102") ) {
      // So basically get a route to the origin (we got the packet from the origin, so that should at least be valid)
      m_routingTable.LookupRoute (origin, toDst);
      Ptr<Ipv4Route> route = Create<Ipv4Route> (*(toDst.GetRoute()));

      // Make sure that we do have that route
      if (toDst.GetRoute()->GetGateway() == Ipv4Address("102.102.102.102")) {
        // Otherwise get a route to bcast (that we do have)
        m_routingTable.LookupRoute ("10.1.31.255", toDst);
        route = Create<Ipv4Route> (*(toDst.GetRoute()));
        m_qlearner->FixRoute(route, m_ipv4->GetObject<Ipv4L3Protocol> ()->GetNetDevice (1) , origin);
      }
      //Set destination correctly
      route->SetDestination(dst);

      NS_ASSERT(route->GetGateway() != Ipv4Address("102.102.102.102") );
      if (!m_qlearner->Route(route, p, header)) { // checked for Delay-Change-Route-Issue
        // checking pkt forwarding counting too many
        NS_FATAL_ERROR("I dont think this actually ever happens because if we have to forward a packet we should indeed also have a neighbour to use.");

        //Drop it, no neighbours = no routes...
        return true;
      }
      ucb (route, p, header);
      // std::cout << "\n-----·\nThe route we were looking for was to " << dst << ". We are " << m_ipv4->GetAddress(1,0).GetLocal() << std::endl;
      // std::cout << m_qlearner->PrintQTable(VOIP) << std::endl;
      // p->Print(std::cout);std::cout<<"  "<<p->GetUid()<<std::endl;
      // std::cout << "what we ended up finding_2:" << *route << std::endl;
      return true;
    }
  }

  NS_LOG_DEBUG ("Route not found to "<< dst << ". Send RERR message. Drop packet " << p->GetUid () << " because no route to forward it_2.");
  NS_LOG_DEBUG("Route:" << *(toDst.GetRoute()) << "   " << toDst.GetFlag() << " (0 is valid___)    " << toDst.GetLifeTime().As(Time::S) << "  " << toDst.GetInterface() );
  std::stringstream ss;p->Print(ss);NS_LOG_DEBUG(ss.str() << "\nat time " << Simulator::Now().As(Time::S));
  NS_LOG_DEBUG("RERR: " << origin << " --> " << dst << " went wrong at node " << m_qlearner->GetNode()->GetId() << " for packet " << p->GetUid());

  SendRerrWhenNoRouteToForward (dst, 0, origin);

  NS_ASSERT(false);

  return false;
}

void
RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_ASSERT (ipv4 != 0);
  NS_ASSERT (m_ipv4 == 0);

  m_ipv4 = ipv4;

  // Create lo route. It is asserted that the only one interface up for now is loopback
  NS_ASSERT (m_ipv4->GetNInterfaces () == 1 && m_ipv4->GetAddress (0, 0).GetLocal () == Ipv4Address ("127.0.0.1"));
  m_lo = m_ipv4->GetNetDevice (0);
  NS_ASSERT (m_lo != 0);
  // Remember lo route
  RoutingTableEntry rt (/*device=*/ m_lo, /*dst=*/ Ipv4Address::GetLoopback (), /*know seqno=*/ true, /*seqno=*/ 0,
                                    /*iface=*/ Ipv4InterfaceAddress (Ipv4Address::GetLoopback (), Ipv4Mask ("255.0.0.0")),
                                    /*hops=*/ 1, /*next hop=*/ Ipv4Address::GetLoopback (),
                                    /*lifetime=*/ Simulator::GetMaximumSimulationTime ());
  m_routingTable.AddRoute (rt);

  Simulator::ScheduleNow (&RoutingProtocol::Start, this);
}

void
RoutingProtocol::NotifyInterfaceUp (uint32_t i)
{
  NS_LOG_FUNCTION (this << m_ipv4->GetAddress (i, 0).GetLocal ());
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  if (l3->GetNAddresses (i) > 1)
    {
      NS_LOG_WARN ("AODV does not work with more then one address per each interface.");
    }
  Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
  if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
    return;

  // Create a socket to listen only on this interface
  Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                             UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvAodv, this));
  socket->Bind (InetSocketAddress (iface.GetLocal (), AODV_PORT));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->SetAllowBroadcast (true);
  socket->SetIpRecvTtl (true);

  m_socketAddresses.insert (std::make_pair (socket, iface));

  // create also a subnet broadcast socket
  socket = Socket::CreateSocket (GetObject<Node> (),
                                 UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvAodv, this));
  socket->Bind (InetSocketAddress (iface.GetBroadcast (), AODV_PORT));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->SetAllowBroadcast (true);
  socket->SetIpRecvTtl (true);
  m_socketSubnetBroadcastAddresses.insert (std::make_pair (socket, iface));

  // Add local broadcast record to the routing table
  Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
  RoutingTableEntry rt (/*device=*/ dev, /*dst=*/ iface.GetBroadcast (), /*know seqno=*/ true, /*seqno=*/ 0, /*iface=*/ iface,
                                    /*hops=*/ 1, /*next hop=*/ iface.GetBroadcast (), /*lifetime=*/ Simulator::GetMaximumSimulationTime ());
  m_routingTable.AddRoute (rt);

  if (l3->GetInterface (i)->GetArpCache ())
    {
      m_nb.AddArpCache (l3->GetInterface (i)->GetArpCache ());
    }

  // Allow neighbor manager use this interface for layer 2 feedback if possible
  Ptr<WifiNetDevice> wifi = dev->GetObject<WifiNetDevice> ();
  if (wifi == 0)
    return;
  Ptr<WifiMac> mac = wifi->GetMac ();
  if (mac == 0)
    return;

  mac->TraceConnectWithoutContext ("TxErrHeader", m_nb.GetTxErrorCallback ());
}

void
RoutingProtocol::NotifyInterfaceDown (uint32_t i)
{
  NS_LOG_FUNCTION (this << m_ipv4->GetAddress (i, 0).GetLocal ());

  // Disable layer 2 link state monitoring (if possible)
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  Ptr<NetDevice> dev = l3->GetNetDevice (i);
  Ptr<WifiNetDevice> wifi = dev->GetObject<WifiNetDevice> ();
  if (wifi != 0)
    {
      Ptr<WifiMac> mac = wifi->GetMac ()->GetObject<AdhocWifiMac> ();
      if (mac != 0)
        {
          mac->TraceDisconnectWithoutContext ("TxErrHeader",
                                              m_nb.GetTxErrorCallback ());
          m_nb.DelArpCache (l3->GetInterface (i)->GetArpCache ());
        }
    }

  // Close socket
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (m_ipv4->GetAddress (i, 0));
  NS_ASSERT (socket);
  socket->Close ();
  m_socketAddresses.erase (socket);

  // Close socket
  socket = FindSubnetBroadcastSocketWithInterfaceAddress (m_ipv4->GetAddress (i, 0));
  NS_ASSERT (socket);
  socket->Close ();
  m_socketSubnetBroadcastAddresses.erase (socket);

  if (m_socketAddresses.empty ())
    {
      NS_LOG_LOGIC ("No aodv interfaces");
      m_htimer.Cancel ();
      m_nb.Clear ();
      m_routingTable.Clear ();
      return;
    }
  m_routingTable.DeleteAllRoutesFromInterface (m_ipv4->GetAddress (i, 0));
}

void
RoutingProtocol::NotifyAddAddress (uint32_t i, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION (this << " interface " << i << " address " << address);
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  if (!l3->IsUp (i))
    return;
  if (l3->GetNAddresses (i) == 1)
    {
      Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
      Ptr<Socket> socket = FindSocketWithInterfaceAddress (iface);
      if (!socket)
        {
          if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
            return;
          // Create a socket to listen only on this interface
          Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                                     UdpSocketFactory::GetTypeId ());
          NS_ASSERT (socket != 0);
          socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvAodv,this));
          socket->Bind (InetSocketAddress (iface.GetLocal (), AODV_PORT));
          socket->BindToNetDevice (l3->GetNetDevice (i));
          socket->SetAllowBroadcast (true);
          m_socketAddresses.insert (std::make_pair (socket, iface));

          // create also a subnet directed broadcast socket
          socket = Socket::CreateSocket (GetObject<Node> (),
                                                       UdpSocketFactory::GetTypeId ());
          NS_ASSERT (socket != 0);
          socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvAodv, this));
          socket->Bind (InetSocketAddress (iface.GetBroadcast (), AODV_PORT));
          socket->BindToNetDevice (l3->GetNetDevice (i));
          socket->SetAllowBroadcast (true);
          socket->SetIpRecvTtl (true);
          m_socketSubnetBroadcastAddresses.insert (std::make_pair (socket, iface));

          // Add local broadcast record to the routing table
          Ptr<NetDevice> dev = m_ipv4->GetNetDevice (
              m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
          RoutingTableEntry rt (/*device=*/ dev, /*dst=*/ iface.GetBroadcast (), /*know seqno=*/ true,
                                            /*seqno=*/ 0, /*iface=*/ iface, /*hops=*/ 1,
                                            /*next hop=*/ iface.GetBroadcast (), /*lifetime=*/ Simulator::GetMaximumSimulationTime ());
          m_routingTable.AddRoute (rt);
        }
    }
  else
    {
      NS_LOG_LOGIC ("AODV does not work with more then one address per each interface. Ignore added address");
    }
}

void
RoutingProtocol::NotifyRemoveAddress (uint32_t i, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION (this);
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (address);
  if (socket)
    {
      m_routingTable.DeleteAllRoutesFromInterface (address);
      socket->Close ();
      m_socketAddresses.erase (socket);

      Ptr<Socket> unicastSocket = FindSubnetBroadcastSocketWithInterfaceAddress (address);
      if (unicastSocket)
        {
          unicastSocket->Close ();
          m_socketAddresses.erase (unicastSocket);
        }

      Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
      if (l3->GetNAddresses (i))
        {
          Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
          // Create a socket to listen only on this interface
          Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                                     UdpSocketFactory::GetTypeId ());
          NS_ASSERT (socket != 0);
          socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvAodv, this));
          // Bind to any IP address so that broadcasts can be received
          socket->Bind (InetSocketAddress (iface.GetLocal (), AODV_PORT));
          socket->BindToNetDevice (l3->GetNetDevice (i));
          socket->SetAllowBroadcast (true);
          socket->SetIpRecvTtl (true);
          m_socketAddresses.insert (std::make_pair (socket, iface));

          // create also a unicast socket
          socket = Socket::CreateSocket (GetObject<Node> (),
                                                       UdpSocketFactory::GetTypeId ());
          NS_ASSERT (socket != 0);
          socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvAodv, this));
          socket->Bind (InetSocketAddress (iface.GetBroadcast (), AODV_PORT));
          socket->BindToNetDevice (l3->GetNetDevice (i));
          socket->SetAllowBroadcast (true);
          socket->SetIpRecvTtl (true);
          m_socketSubnetBroadcastAddresses.insert (std::make_pair (socket, iface));

          // Add local broadcast record to the routing table
          Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
          RoutingTableEntry rt (/*device=*/ dev, /*dst=*/ iface.GetBroadcast (), /*know seqno=*/ true, /*seqno=*/ 0, /*iface=*/ iface,
                                            /*hops=*/ 1, /*next hop=*/ iface.GetBroadcast (), /*lifetime=*/ Simulator::GetMaximumSimulationTime ());
          m_routingTable.AddRoute (rt);
        }
      if (m_socketAddresses.empty ())
        {
          NS_LOG_LOGIC ("No aodv interfaces");
          m_htimer.Cancel ();
          m_nb.Clear ();
          m_routingTable.Clear ();
          return;
        }
    }
  else
    {
      NS_LOG_LOGIC ("Remove address not participating in AODV operation");
    }
}

bool
RoutingProtocol::IsMyOwnAddress (Ipv4Address src)
{
  NS_LOG_FUNCTION (this << src);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ipv4InterfaceAddress iface = j->second;
      if (src == iface.GetLocal ())
        {
          return true;
        }
    }
  return false;
}

Ptr<Ipv4Route>
RoutingProtocol::LoopbackRoute (const Ipv4Header & hdr, Ptr<NetDevice> oif) const
{
  NS_LOG_FUNCTION (this << hdr);
  NS_ASSERT (m_lo != 0);
  Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
  rt->SetDestination (hdr.GetDestination ());
  //
  // Source address selection here is tricky.  The loopback route is
  // returned when AODV does not have a route; this causes the packet
  // to be looped back and handled (cached) in RouteInput() method
  // while a route is found. However, connection-oriented protocols
  // like TCP need to create an endpoint four-tuple (src, src port,
  // dst, dst port) and create a pseudo-header for checksumming.  So,
  // AODV needs to guess correctly what the eventual source address
  // will be.
  //
  // For single interface, single address nodes, this is not a problem.
  // When there are possibly multiple outgoing interfaces, the policy
  // implemented here is to pick the first available AODV interface.
  // If RouteOutput() caller specified an outgoing interface, that
  // further constrains the selection of source address
  //
  std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
  if (oif)
    {
      // Iterate to find an address on the oif device
      for (j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
        {
          Ipv4Address addr = j->second.GetLocal ();
          int32_t interface = m_ipv4->GetInterfaceForAddress (addr);
          if (oif == m_ipv4->GetNetDevice (static_cast<uint32_t> (interface)))
            {
              rt->SetSource (addr);
              break;
            }
        }
    }
  else
    {
      rt->SetSource (j->second.GetLocal ());
    }
  NS_ASSERT_MSG (rt->GetSource () != Ipv4Address (), "Valid AODV source address not found");
  rt->SetGateway (Ipv4Address ("127.0.0.1"));
  rt->SetOutputDevice (m_lo);

  return rt;
}

void
RoutingProtocol::SendRequest (Ipv4Address dst)
{
  NS_LOG_FUNCTION ( this << dst);
  // A node SHOULD NOT originate more than RREQ_RATELIMIT RREQ messages per second.
  if (m_rreqCount == m_rreqRateLimit)
    {
      Simulator::Schedule (m_rreqRateLimitTimer.GetDelayLeft () + MicroSeconds (100),
                           &RoutingProtocol::SendRequest, this, dst);
      return;
    }
  else
    m_rreqCount++;
  // Create RREQ header
  RreqHeader rreqHeader;
  rreqHeader.SetDst (dst);

  RoutingTableEntry rt;
  // Using the Hop field in Routing Table to manage the expanding ring search
  uint16_t ttl = m_ttlStart;
  if (m_routingTable.LookupRoute (dst, rt))
    {
      if (rt.GetFlag () != IN_SEARCH)
        {
          ttl = std::min<uint16_t> (rt.GetHop () + m_ttlIncrement, m_netDiameter);
        }
      else
        {
          ttl = rt.GetHop () + m_ttlIncrement;
          if (ttl > m_ttlThreshold)
            ttl = m_netDiameter;
        }
      if (ttl == m_netDiameter)
        rt.IncrementRreqCnt ();
      if (rt.GetValidSeqNo ())
        rreqHeader.SetDstSeqno (rt.GetSeqNo ());
      else
        rreqHeader.SetUnknownSeqno (true);
      rt.SetHop (ttl);
      rt.SetFlag (IN_SEARCH);
      rt.SetLifeTime (m_pathDiscoveryTime);
      m_routingTable.Update (rt);
    }
  else
    {
      rreqHeader.SetUnknownSeqno (true);
      Ptr<NetDevice> dev = 0;
      RoutingTableEntry newEntry (/*device=*/ dev, /*dst=*/ dst, /*validSeqNo=*/ false, /*seqno=*/ 0,
                                              /*iface=*/ Ipv4InterfaceAddress (),/*hop=*/ ttl,
                                              /*nextHop=*/ Ipv4Address (), /*lifeTime=*/ m_pathDiscoveryTime);
      // Check if TtlStart == NetDiameter
      if (ttl == m_netDiameter)
        newEntry.IncrementRreqCnt ();
      newEntry.SetFlag (IN_SEARCH);
      m_routingTable.AddRoute (newEntry);
      /**
       * If we are sending a RouteRequest it means we have a reason / need to contact a certain destination IP
       * As we are sending a RREQ, we dont have a route / next-hop available to us. Thus, the via parameter will be set to IP_WHEN_NO_NEXT_HOP_NEIGHBOUR_KNOWN_YET
       *
       * The AddDestination function will check for that value and make sure all values are initialised to 0 only.
       *
       * ADD_DST_CASE_2
       */
      NS_LOG_DEBUG ("ADD_DST_CASE_2  ---  " << Ipv4Address(IP_WHEN_NO_NEXT_HOP_NEIGHBOUR_KNOWN_YET) << "  " << dst );

      if (m_qlearner) {
        if (m_qlearner->AddDestination(Ipv4Address(IP_WHEN_NO_NEXT_HOP_NEIGHBOUR_KNOWN_YET), dst, Seconds(0))) {
          NS_LOG_DEBUG ("ADD_DST_CASE_2  TRUE (new dst)");
        } else {
          NS_LOG_DEBUG ("ADD_DST_CASE_2  FALSE (already known)" );
        }
      }
    }

  if (m_gratuitousReply)
	  rreqHeader.SetGratuitousRrep(true);
  if (m_destinationOnly)
    rreqHeader.SetDestinationOnly (true);

  m_seqNo++;
  rreqHeader.SetOriginSeqno (m_seqNo);
  m_requestId++;
  rreqHeader.SetId (m_requestId);

  // Send RREQ as subnet directed broadcast from each interface used by aodv
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;

      rreqHeader.SetOrigin (iface.GetLocal ());
      m_rreqIdCache.IsDuplicate (iface.GetLocal (), m_requestId);

      Ptr<Packet> packet = Create<Packet> ();
      SocketIpTtlTag tag;
      tag.SetTtl (ttl);
      packet->AddPacketTag (tag);
      packet->AddHeader (rreqHeader);
      TypeHeader tHeader (AODVTYPE_RREQ);
      packet->AddHeader (tHeader);
      // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
      Ipv4Address destination;
      if (iface.GetMask () == Ipv4Mask::GetOnes ())
        {
          destination = Ipv4Address ("255.255.255.255");
        }
      else
        {
          destination = iface.GetBroadcast ();
        }
      NS_LOG_DEBUG ("Send RREQ with id " << rreqHeader.GetId () << " and TTL " << ttl<< " to socket. Packet ID: " << packet->GetUid());
      m_lastBcastTime = Simulator::Now ();
      Simulator::Schedule (Time (MilliSeconds (m_uniformRandomVariable->GetInteger (0, 10))), &RoutingProtocol::SendTo, this, socket, packet, destination);
    }
  ScheduleRreqRetry (dst);
}

void
RoutingProtocol::SendTo (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination)
{
  // In this case, no need to specify port number, it's a full AODV packet and will be seen as such
  PacketTrackingOutput(packet);
  socket->SendTo (packet, 0, InetSocketAddress (destination, AODV_PORT));
}
void
RoutingProtocol::ScheduleRreqRetry (Ipv4Address dst)
{
  NS_LOG_FUNCTION (this << dst);
  if (m_addressReqTimer.find (dst) == m_addressReqTimer.end ())
    {
      Timer timer (Timer::CANCEL_ON_DESTROY);
      m_addressReqTimer[dst] = timer;
    }
  m_addressReqTimer[dst].SetFunction (&RoutingProtocol::RouteRequestTimerExpire, this);
  m_addressReqTimer[dst].Remove ();
  m_addressReqTimer[dst].SetArguments (dst);
  RoutingTableEntry rt;
  m_routingTable.LookupRoute (dst, rt);
  Time retry;
  if (rt.GetHop () < m_netDiameter)
    {
      retry = 2 * m_nodeTraversalTime * (rt.GetHop () + m_timeoutBuffer);
    }
  else
  {
      NS_ABORT_MSG_UNLESS (rt.GetRreqCnt () > 0, "Unexpected value for GetRreqCount ()");
      uint16_t backoffFactor = rt.GetRreqCnt () - 1;
      NS_LOG_LOGIC ("Applying binary exponential backoff factor " << backoffFactor);
      retry = m_netTraversalTime * (1 << backoffFactor);
  }
  m_addressReqTimer[dst].Schedule (retry);
  NS_LOG_LOGIC ("Scheduled RREQ retry in " << retry.GetSeconds () << " seconds");
}

void RoutingProtocol::RecvAodvWithoutRead (Ptr<Socket> socket, Ptr<Packet> packet, Address sourceAddress) {
  InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
  Ipv4Address sender = inetSourceAddr.GetIpv4 ();
  Ipv4Address receiver;
  if (m_socketAddresses.find (socket) != m_socketAddresses.end ())
    {
      receiver = m_socketAddresses[socket].GetLocal ();
    }
  else if(m_socketSubnetBroadcastAddresses.find (socket) != m_socketSubnetBroadcastAddresses.end ())
    {
      receiver = m_socketSubnetBroadcastAddresses[socket].GetLocal ();
    }
  else
    {
      NS_ASSERT_MSG (false, "Received a packet from an unknown socket");
    }
  NS_LOG_DEBUG ("AODV node " << this << " received a AODV packet from " << sender << " to " << receiver);

  UpdateRouteToNeighbor (sender, receiver);
  TypeHeader tHeader (AODVTYPE_RREQ);
  packet->RemoveHeader (tHeader);
  if (!tHeader.IsValid ())
    {
      NS_LOG_DEBUG ("AODV message " << packet->GetUid () << " with unknown type received: " << tHeader.Get () << ". Drop");
      return; // drop
    }
  switch (tHeader.Get ())
    {
    case AODVTYPE_RREQ:
      {
        RecvRequest (packet, receiver, sender);
        break;
      }
    case AODVTYPE_RREP:
      {
        RecvReply (packet, receiver, sender);
        break;
      }
    case AODVTYPE_RERR:
      {
        RecvError (packet, sender);
        break;
      }
    case AODVTYPE_RREP_ACK:
      {
        RecvReplyAck (sender);
        break;
      }
    }
}

void
RoutingProtocol::RecvAodv (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Address sourceAddress;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
  InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
  Ipv4Address sender = inetSourceAddr.GetIpv4 ();
  Ipv4Address receiver;

  if (m_socketAddresses.find (socket) != m_socketAddresses.end ())
    {
      receiver = m_socketAddresses[socket].GetLocal ();
    }
  else if(m_socketSubnetBroadcastAddresses.find (socket) != m_socketSubnetBroadcastAddresses.end ())
    {
      receiver = m_socketSubnetBroadcastAddresses[socket].GetLocal ();
    }
  else
    {
      NS_ASSERT_MSG (false, "Received a packet from an unknown socket");
    }
  NS_LOG_DEBUG ("AODV node " << this << " received a AODV packet from " << sender << " to " << receiver);

  UpdateRouteToNeighbor (sender, receiver);

  TypeHeader tHeader (AODVTYPE_RREQ);
  packet->RemoveHeader (tHeader);
  if (!tHeader.IsValid ())
    {
      NS_LOG_DEBUG ("AODV message " << packet->GetUid () << " with unknown type received: " << tHeader.Get () << ". Drop");
      return; // drop
    }
  switch (tHeader.Get ())
    {
    case AODVTYPE_RREQ:
      {
        NS_LOG_DEBUG ("RREQ received");
        RecvRequest (packet, receiver, sender);
        break;
      }
    case AODVTYPE_RREP:
      {
        NS_LOG_DEBUG ("RREP received in packet " << packet->GetUid());
        RecvReply (packet, receiver, sender);
        break;
      }
    case AODVTYPE_RERR:
      {
        NS_LOG_DEBUG ("RERR received from " << sender <<" in packet "<< packet->GetUid());
        RecvError (packet, sender);
        break;
      }
    case AODVTYPE_RREP_ACK:
      {
        NS_LOG_DEBUG ("REPACK received");
        RecvReplyAck (sender);
        break;
      }
    }
}

bool
RoutingProtocol::UpdateRouteLifeTime (Ipv4Address addr, Time lifetime)
{
  NS_LOG_FUNCTION (this << addr << lifetime);
  RoutingTableEntry rt;
  if (m_routingTable.LookupRoute (addr, rt))
    {
      if (rt.GetFlag () == VALID)
        {
          // NS_LOG_DEBUG ("Updating VALID route");
          rt.SetRreqCnt (0);
          rt.SetLifeTime (std::max (lifetime, rt.GetLifeTime ()));
          m_routingTable.Update (rt);
          return true;
        }
    }
  return false;
}

void
RoutingProtocol::UpdateRouteToNeighbor (Ipv4Address sender, Ipv4Address receiver)
{
  NS_LOG_FUNCTION (this << "sender " << sender << " receiver " << receiver);
  RoutingTableEntry toNeighbor;
  if (!m_routingTable.LookupRoute (sender, toNeighbor))
    {
      Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (receiver));
      RoutingTableEntry newEntry (/*device=*/ dev, /*dst=*/ sender, /*know seqno=*/ false, /*seqno=*/ 0,
                                              /*iface=*/ m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0),
                                              /*hops=*/ 1, /*next hop=*/ sender, /*lifetime=*/ m_activeRouteTimeout);
      m_routingTable.AddRoute (newEntry);
      /**
       * We received an AODV Packet, and are trying to update route to neighbour, this happens in any case where
       * a packet arrives on the port for AODV (654)
       * and it is as good a time as any to register the dst / neighbour
       *
       *
       * The AddDestination function will check for that value and make sure all values are initialised to 0 only.
       *
       * ADD_DST_CASE_3
       */
      NS_LOG_DEBUG ("ADD_DST_CASE_3  ---  " << Ipv4Address(IP_WHEN_NO_NEXT_HOP_NEIGHBOUR_KNOWN_YET) << "  " << sender );
      if (m_qlearner) {
        m_qlearner->AddDestination(Ipv4Address(IP_WHEN_NO_NEXT_HOP_NEIGHBOUR_KNOWN_YET), sender, Seconds(0));
      }
    }
  else
    {
      Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (receiver));
      if (toNeighbor.GetValidSeqNo () && (toNeighbor.GetHop () == 1) && (toNeighbor.GetOutputDevice () == dev))
        {
          toNeighbor.SetLifeTime (std::max (m_activeRouteTimeout, toNeighbor.GetLifeTime ()));
        }
      else
        {
          RoutingTableEntry newEntry (/*device=*/ dev, /*dst=*/ sender, /*know seqno=*/ false, /*seqno=*/ 0,
                                                  /*iface=*/ m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0),
                                                  /*hops=*/ 1, /*next hop=*/ sender, /*lifetime=*/ std::max (m_activeRouteTimeout, toNeighbor.GetLifeTime ()));


          if (m_qlearner) {
            NS_ASSERT_MSG(m_qlearner->CheckDestinationKnown(sender), "AODV is trying to update a route to some destination and we are part of the path (fine).\
                                                                BUT SOMEHOW the QTable does not know about this (not fine)!");
          }
          m_routingTable.Update (newEntry);
        }
    }

}

void
RoutingProtocol::RecvRequest (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src)
{
  NS_LOG_FUNCTION (this);
  RreqHeader rreqHeader;
  p->RemoveHeader (rreqHeader);

  // A node ignores all RREQs received from any node in its blacklist
  RoutingTableEntry toPrev;
  if (m_routingTable.LookupRoute (src, toPrev))
    {
      if (toPrev.IsUnidirectional ())
        {
          NS_LOG_DEBUG ("Ignoring RREQ from node in blacklist");
          return;
        }
    }

  uint32_t id = rreqHeader.GetId ();
  Ipv4Address origin = rreqHeader.GetOrigin ();

  if (m_qlearner) {
    // If a QLearner sees an AODV Hello, clearly, the node is a neighbour and it should be _SURE_ that that node is amongst the QLearner's available neighbours list
    // If that is not already the case, it should be added.
    m_qlearner->AddNeighbour( src );
    // std::cout << m_ipv4->GetAddress(1,0).GetLocal() << " adding " << src << " as neighbour due to a RREQ pkt.\n";
  }


  /*
   *  Node checks to determine whether it has received a RREQ with the same Originator IP Address and RREQ ID.
   *  If such a RREQ has been received, the node silently discards the newly received RREQ.
   */
  if (m_rreqIdCache.IsDuplicate (origin, id))
    {
      NS_LOG_DEBUG ("Ignoring RREQ due to duplicate");
      return;
    }

  // Increment RREQ hop count
  uint8_t hop = rreqHeader.GetHopCount () + 1;
  rreqHeader.SetHopCount (hop);

  /*
   *  When the reverse route is created or updated, the following actions on the route are also carried out:
   *  1. the Originator Sequence Number from the RREQ is compared to the corresponding destination sequence number
   *     in the route table entry and copied if greater than the existing value there
   *  2. the valid sequence number field is set to true;
   *  3. the next hop in the routing table becomes the node from which the  RREQ was received
   *  4. the hop count is copied from the Hop Count in the RREQ message;
   *  5. the Lifetime is set to be the maximum of (ExistingLifetime, MinimalLifetime), where
   *     MinimalLifetime = current time + 2*NetTraversalTime - 2*HopCount*NodeTraversalTime
   */
  RoutingTableEntry toOrigin;
  if (!m_routingTable.LookupRoute (origin, toOrigin))
    {
      Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (receiver));
      RoutingTableEntry newEntry (/*device=*/ dev, /*dst=*/ origin, /*validSeno=*/ true, /*seqNo=*/ rreqHeader.GetOriginSeqno (),
                                              /*iface=*/ m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0), /*hops=*/ hop,
                                              /*nextHop*/ src, /*timeLife=*/ Time ((2 * m_netTraversalTime - 2 * hop * m_nodeTraversalTime)));
      m_routingTable.AddRoute (newEntry);
      /**
       * We received a RREQ Packet, so this means we can add _3_ destinations! (here we do 1 and 2)
       * The originator of the RREQ packet is a possible destination we now know about and so is the eventual receiver / destination
       * This is the "route" and the "reverse route"
       *
       * In this case, via should be set correctly for the path to the origin
       *
       * Otherwise, this has to be done for the RREP_ACK but thats not for certain always gonna happen,
       * while setting next_hop / via for reverse route (route back to origin) should work now, the RREQ somehow reached us
       * and it will somehow reach back to origin if we send it back that way
       *
       * For destination we dont know for sure yet, so it becomes IP_WHEN_NO_NEXT_HOP_NEIGHBOUR_KNOWN_YET as a via
       *
       * (received as a broadcast packet)
       *
       * Clearly, we should only add the destination if it is not the same as our ip address!
       *
       * ADD_DST_CASE_4
       */
      NS_LOG_DEBUG ("ADD_DST_CASE_4  ---  " << src << "  " << origin );
      NS_LOG_DEBUG ("RREQ DST : " << rreqHeader.GetDst() << " " << " and sender is " << origin << " . We received it from " << src);
      if (m_qlearner) {
        if (origin != m_ipv4->GetAddress(1,0).GetLocal()) {
          // Added this if clause for the case where a RREQ is sent, it is sent back to the source and the source tries to add the prev_hop via itself
          m_qlearner->AddDestination(src, origin, Seconds(0));
        }
        if (rreqHeader.GetDst() != receiver) {
          m_qlearner->AddDestination(Ipv4Address(IP_WHEN_NO_NEXT_HOP_NEIGHBOUR_KNOWN_YET), rreqHeader.GetDst (), Seconds(0));
        }
      }
    }
  else
    {
      if (toOrigin.GetValidSeqNo ())
        {
          if (int32_t (rreqHeader.GetOriginSeqno ()) - int32_t (toOrigin.GetSeqNo ()) > 0) {
            toOrigin.SetSeqNo (rreqHeader.GetOriginSeqno ());
          }
        }
      else
        toOrigin.SetSeqNo (rreqHeader.GetOriginSeqno ());
      toOrigin.SetValidSeqNo (true);
      toOrigin.SetNextHop (src);
      toOrigin.SetOutputDevice (m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (receiver)));
      toOrigin.SetInterface (m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0));
      toOrigin.SetHop (hop);
      toOrigin.SetLifeTime (std::max (Time (2 * m_netTraversalTime - 2 * hop * m_nodeTraversalTime),
                                      toOrigin.GetLifeTime ()));
      m_routingTable.Update (toOrigin);
      //m_nb.Update (src, Time (AllowedHelloLoss * HelloInterval));
    }


  RoutingTableEntry toNeighbor;
  if (!m_routingTable.LookupRoute (src, toNeighbor))
    {
      NS_LOG_DEBUG ("ADD_DST_CASE_5  ---  " << src << "  " << src );
      NS_LOG_DEBUG ("Neighbor:" << src << " not found in routing table. Creating an entry");
      Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (receiver));
      RoutingTableEntry newEntry (dev, src, false, rreqHeader.GetOriginSeqno (),
                                              m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0),
                                              1, src, m_activeRouteTimeout);
      m_routingTable.AddRoute (newEntry);
      /**
       * We received a RREQ Packet, so this means we can add _3_ destinations! (here we do 3)
       *
       * Third case is the prev-hop neighbour that sent the RREQ. Ideally, it is already noted as a neighbour but it can't hurt
       * to make sure of this and to check it..
       *
       * ADD_DST_CASE_5
       */
      if (m_qlearner) {
        m_qlearner->AddDestination(src, src, Seconds(0));
      }
    }
  else
    {
      toNeighbor.SetLifeTime (m_activeRouteTimeout);
      toNeighbor.SetValidSeqNo (false);
      toNeighbor.SetSeqNo (rreqHeader.GetOriginSeqno ());
      toNeighbor.SetFlag (VALID);
      toNeighbor.SetOutputDevice (m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (receiver)));
      toNeighbor.SetInterface (m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0));
      toNeighbor.SetHop (1);
      toNeighbor.SetNextHop (src);
      m_routingTable.Update (toNeighbor);
    }
  m_nb.Update (src, Time (m_allowedHelloLoss * m_helloInterval));

  NS_LOG_DEBUG (receiver << " receive RREQ with hop count " << static_cast<uint32_t>(rreqHeader.GetHopCount ())
                         << " ID " << rreqHeader.GetId ()
                         << " to destination " << rreqHeader.GetDst ()
                         << " in packet id " << p->GetUid());

  //  A node generates a RREP if either:
  //  (i)  it is itself the destination,
  if (IsMyOwnAddress (rreqHeader.GetDst ()))
    {
      m_routingTable.LookupRoute (origin, toOrigin);
      NS_LOG_DEBUG ("Send reply since I am the destination. RREQ was in Packet ID : " << p->GetUid());
      SendReply (rreqHeader, toOrigin);
      return;
    }
  /*
   * (ii) or it has an active route to the destination, the destination sequence number in the node's existing route table entry for the destination
   *      is valid and greater than or equal to the Destination Sequence Number of the RREQ, and the "destination only" flag is NOT set.
   */
  RoutingTableEntry toDst;
  Ipv4Address dst = rreqHeader.GetDst ();
  if (m_routingTable.LookupRoute (dst, toDst))
    {
      /*
       * Drop RREQ, This node RREP wil make a loop.
       */
      if (toDst.GetNextHop () == src)
        {
          NS_LOG_DEBUG ("Drop RREQ from " << src << ", dest next hop " << toDst.GetNextHop ());
          return;
        }
      /*
       * The Destination Sequence number for the requested destination is set to the maximum of the corresponding value
       * received in the RREQ message, and the destination sequence value currently maintained by the node for the requested destination.
       * However, the forwarding node MUST NOT modify its maintained value for the destination sequence number, even if the value
       * received in the incoming RREQ is larger than the value currently maintained by the forwarding node.
       */
      // NS_LOG_DEBUG("rreqHeader unknownseqno " << rreqHeader.GetUnknownSeqno () << false);
      // NS_LOG_DEBUG("2nd " << (int32_t (toDst.GetSeqNo ()) - int32_t (rreqHeader.GetDstSeqno ()) >= 0) << true);
      // NS_LOG_DEBUG("&& this " << toDst.GetValidSeqNo () );
      // This makes a difference of 1 packet extra being exchanged in test0 (and others, but thats where I noticed it)
      // Unsure why this is but its AODV which should just be used to bootstrap

      if ((rreqHeader.GetUnknownSeqno () || (int32_t (toDst.GetSeqNo ()) - int32_t (rreqHeader.GetDstSeqno ()) >= 0))
          && toDst.GetValidSeqNo () )
        {
          NS_LOG_DEBUG("Maybe reply for intermediate node?");
          if (!rreqHeader.GetDestinationOnly () && toDst.GetFlag () == VALID)
            {
              m_routingTable.LookupRoute (origin, toOrigin);
              NS_LOG_DEBUG("Replying for intermediate node.");
              SendReplyByIntermediateNode (toDst, toOrigin, rreqHeader.GetGratuitousRrep());
              return;
            }
          rreqHeader.SetDstSeqno (toDst.GetSeqNo ());
          rreqHeader.SetUnknownSeqno (false);
        }
    }

  SocketIpTtlTag tag;
  p->RemovePacketTag (tag);
  if (tag.GetTtl () < 2)
    {
      NS_LOG_DEBUG ("TTL exceeded. Drop RREQ origin " << src << " destination " << dst );
      return;
    }

  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      Ptr<Packet> packet = Create<Packet> ();
      SocketIpTtlTag ttl;
      ttl.SetTtl (tag.GetTtl () - 1);
      packet->AddPacketTag (ttl);
      packet->AddHeader (rreqHeader);
      TypeHeader tHeader (AODVTYPE_RREQ);
      packet->AddHeader (tHeader);
      // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
      Ipv4Address destination;
      if (iface.GetMask () == Ipv4Mask::GetOnes ())
        {
          destination = Ipv4Address ("255.255.255.255");
        }
      else
        {
          destination = iface.GetBroadcast ();
        }
      m_lastBcastTime = Simulator::Now ();
      Simulator::Schedule (Time (MilliSeconds (m_uniformRandomVariable->GetInteger (0, 10))), &RoutingProtocol::SendTo, this, socket, packet, destination);

    }
}

void
RoutingProtocol::SendReply (RreqHeader const & rreqHeader, RoutingTableEntry const & toOrigin)
{
  NS_LOG_FUNCTION (this << toOrigin.GetDestination ());
  /*
   * Destination node MUST increment its own sequence number by one if the sequence number in the RREQ packet is equal to that
   * incremented value. Otherwise, the destination does not change its sequence number before generating the  RREP message.
   */
  if (!rreqHeader.GetUnknownSeqno () && (rreqHeader.GetDstSeqno () == m_seqNo + 1))
    m_seqNo++;
  RrepHeader rrepHeader ( /*prefixSize=*/ 0, /*hops=*/ 0, /*dst=*/ rreqHeader.GetDst (),
                                          /*dstSeqNo=*/ m_seqNo, /*origin=*/ toOrigin.GetDestination (), /*lifeTime=*/ m_myRouteTimeout);
  Ptr<Packet> packet = Create<Packet> ();
  SocketIpTtlTag tag;
  tag.SetTtl (toOrigin.GetHop ());

  packet->AddPacketTag (tag);
  packet->AddHeader (rrepHeader);
  TypeHeader tHeader (AODVTYPE_RREP);
  packet->AddHeader (tHeader);
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (toOrigin.GetInterface ());
  NS_ASSERT (socket);
  socket->SendTo (packet, 0, InetSocketAddress (toOrigin.GetNextHop (), AODV_PORT));
}

void
RoutingProtocol::SendReplyByIntermediateNode (RoutingTableEntry & toDst, RoutingTableEntry & toOrigin, bool gratRep)
{
  NS_LOG_FUNCTION (this);
  RrepHeader rrepHeader (/*prefix size=*/ 0, /*hops=*/ toDst.GetHop (), /*dst=*/ toDst.GetDestination (), /*dst seqno=*/ toDst.GetSeqNo (),
                                          /*origin=*/ toOrigin.GetDestination (), /*lifetime=*/ toDst.GetLifeTime ());
  /* If the node we received a RREQ for is a neighbor we are
   * probably facing a unidirectional link... Better request a RREP-ack
   */
  if (toDst.GetHop () == 1)
    {
      rrepHeader.SetAckRequired (true);
      RoutingTableEntry toNextHop;
      m_routingTable.LookupRoute (toOrigin.GetNextHop (), toNextHop);
      toNextHop.m_ackTimer.SetFunction (&RoutingProtocol::AckTimerExpire, this);
      toNextHop.m_ackTimer.SetArguments (toNextHop.GetDestination (), m_blackListTimeout);
      toNextHop.m_ackTimer.SetDelay (m_nextHopWait);
    }
  toDst.InsertPrecursor (toOrigin.GetNextHop ());
  toOrigin.InsertPrecursor (toDst.GetNextHop ());
  m_routingTable.Update (toDst);
  m_routingTable.Update (toOrigin);

  Ptr<Packet> packet = Create<Packet> ();
  SocketIpTtlTag tag;
  tag.SetTtl (toOrigin.GetHop ());
  packet->AddPacketTag (tag);
  packet->AddHeader (rrepHeader);

  if (m_qlearner) { //hier is verandering owv error in test0
    NS_LOG_DEBUG(m_ipv4->GetAddress(1,0).GetLocal() << " is sending Reply by imediate for " << rrepHeader.GetDst() << "  via  " << toOrigin.GetNextHop () << "  to  " << toOrigin.GetDestination() );
    m_qlearner->ChangeQValuesFromZero(rrepHeader.GetDst(), toDst.GetNextHop () );
  }

  TypeHeader tHeader (AODVTYPE_RREP);
  packet->AddHeader (tHeader);
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (toOrigin.GetInterface ());
  NS_ASSERT (socket);
  socket->SendTo (packet, 0, InetSocketAddress (toOrigin.GetNextHop (), AODV_PORT));

  // Generating gratuitous RREPs
  if (gratRep)
    {
      RrepHeader gratRepHeader (/*prefix size=*/ 0, /*hops=*/ toOrigin.GetHop (), /*dst=*/ toOrigin.GetDestination (),
                                                 /*dst seqno=*/ toOrigin.GetSeqNo (), /*origin=*/ toDst.GetDestination (),
                                                 /*lifetime=*/ toOrigin.GetLifeTime ());
      Ptr<Packet> packetToDst = Create<Packet> ();
      SocketIpTtlTag gratTag;
      gratTag.SetTtl (toDst.GetHop ());
      packetToDst->AddPacketTag (gratTag);
      packetToDst->AddHeader (gratRepHeader);
      TypeHeader type (AODVTYPE_RREP);
      packetToDst->AddHeader (type);
      Ptr<Socket> socket = FindSocketWithInterfaceAddress (toDst.GetInterface ());
      NS_ASSERT (socket);
      NS_LOG_DEBUG ("Send gratuitous RREP, 1st RREP was in " << packet->GetUid () << " and second is in " << packetToDst->GetUid() << ". Time is: " <<Simulator::Now());
      socket->SendTo (packetToDst, 0, InetSocketAddress (toDst.GetNextHop (), AODV_PORT));
    }
}

void
RoutingProtocol::SendReplyAck (Ipv4Address neighbor)
{
  NS_LOG_FUNCTION (this << " to " << neighbor);
  RrepAckHeader h;
  TypeHeader typeHeader (AODVTYPE_RREP_ACK);
  Ptr<Packet> packet = Create<Packet> ();
  SocketIpTtlTag tag;
  tag.SetTtl (1);
  packet->AddPacketTag (tag);
  packet->AddHeader (h);
  packet->AddHeader (typeHeader);
  RoutingTableEntry toNeighbor;
  m_routingTable.LookupRoute (neighbor, toNeighbor);
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (toNeighbor.GetInterface ());
  NS_ASSERT (socket);
  socket->SendTo (packet, 0, InetSocketAddress (neighbor, AODV_PORT));
}

void
RoutingProtocol::RecvReply (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address sender)
{
  NS_LOG_FUNCTION (this << " src " << sender);
  RrepHeader rrepHeader;
  p->RemoveHeader (rrepHeader);
  Ipv4Address dst = rrepHeader.GetDst ();
  NS_LOG_DEBUG ("RREP destination " << dst << " RREP origin " << rrepHeader.GetOrigin ());

  uint8_t hop = rrepHeader.GetHopCount () + 1;
  rrepHeader.SetHopCount (hop);

  // If RREP is Hello message
  if (dst == rrepHeader.GetOrigin ())
    {
      ProcessHello (rrepHeader, receiver);
      return;
    }

  if (m_qlearner) {
    // std::cout << "hi, i am node" << m_qlearner->GetNode()->GetId() << " origin = "<< rrepHeader.GetOrigin () << "  receiver=" << receiver << "sender?= "<< sender  << std::endl;
    // If a QLearner sees an AODV Hello, clearly, the node is a neighbour and it should be _SURE_ that that node is amongst the QLearner's available neighbours list
    // If that is not already the case, it should be added.
    m_qlearner->AddNeighbour( sender );
    // std::cout << m_ipv4->GetAddress(1,0).GetLocal() << " adding " << sender << " as neighbour due to a RREP (not hello) pkt .\n";
  }


  /*
   * If the route table entry to the destination is created or updated, then the following actions occur:
   * -  the route is marked as active,
   * -  the destination sequence number is marked as valid,
   * -  the next hop in the route entry is assigned to be the node from which the RREP is received,
   *    which is indicated by the source IP address field in the IP header,
   * -  the hop count is set to the value of the hop count from RREP message + 1
   * -  the expiry time is set to the current time plus the value of the Lifetime in the RREP message,
   * -  and the destination sequence number is the Destination Sequence Number in the RREP message.
   */
  Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (receiver));
  RoutingTableEntry newEntry (/*device=*/ dev, /*dst=*/ dst, /*validSeqNo=*/ true, /*seqno=*/ rrepHeader.GetDstSeqno (),
                                          /*iface=*/ m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0),/*hop=*/ hop,
                                          /*nextHop=*/ sender, /*lifeTime=*/ rrepHeader.GetLifeTime ());
  RoutingTableEntry toDst;

  QLrnInfoTag ptst_tag;

  if (m_routingTable.LookupRoute (dst, toDst))
    {
      /*
       * The existing entry is updated only in the following circumstances:
       * (i) the sequence number in the routing table is marked as invalid in route table entry.
       */
      if (!toDst.GetValidSeqNo ())
        {
          m_routingTable.Update (newEntry);
        }
      // (ii)the Destination Sequence Number in the RREP is greater than the node's copy of the destination sequence number and the known value is valid,
      else if ((int32_t (rrepHeader.GetDstSeqno ()) - int32_t (toDst.GetSeqNo ())) > 0)
        {
          m_routingTable.Update (newEntry);
        }
      else
        {
          // (iii) the sequence numbers are the same, but the route is marked as inactive.
          if ((rrepHeader.GetDstSeqno () == toDst.GetSeqNo ()) && (toDst.GetFlag () != VALID))
            {
              m_routingTable.Update (newEntry);
            }
          // (iv)  the sequence numbers are the same, and the New Hop Count is smaller than the hop count in route table entry.
          else if ((rrepHeader.GetDstSeqno () == toDst.GetSeqNo ()) && (hop < toDst.GetHop ()))
            {
              m_routingTable.Update (newEntry);
            }
        }
    }
  else
    {
      // The forward route for this destination is created if it does not already exist.
      m_routingTable.AddRoute (newEntry);

      /**
       * We received a RREP packet, so we add whatever the destination is
       * we assert that source should be known
       *
       * ADD_DST_CASE_6 : Received RREP
       */
      NS_LOG_DEBUG ("ADD_DST_CASE_6  ---  via= " << sender << " dst= " << dst );
      if (m_qlearner) {
        /**
         * We received a RREP Packet.. actually, the destination should already be known!
         * ^incorrect assumption, neighbour of the dst will have a route to dst and will then reply to the RREQ...
         *
         */
        // NS_ASSERT(m_qlearner->CheckDestinationKnown(dst));
          NS_ASSERT(m_qlearner->CheckDestinationKnown(sender));

        /* use PTST data to get a good first guess */
        if (!p->PeekPacketTag(ptst_tag)) {
          NS_LOG_DEBUG("RREP without PTST TAG found. currNode=" << m_ipv4->GetAddress(1,0).GetLocal() << "  dst?  " << rrepHeader.GetOrigin () << " sender:" << sender << " pkt id:" << p->GetUid());
        } else {
          NS_LOG_DEBUG("RREP with PTST TAG found. currNode=" << m_ipv4->GetAddress(1,0).GetLocal() << "  dst?  " << rrepHeader.GetOrigin () << " sender:" << sender << " pkt id:" << p->GetUid() );
          m_qlearner->AddDestination(sender, dst, Simulator::Now() - ptst_tag.GetTime());
        }

      }
    }
  // Acknowledge receipt of the RREP by sending a RREP-ACK message back
  if (rrepHeader.GetAckRequired ())
    {
      SendReplyAck (sender);
      rrepHeader.SetAckRequired (false);
    }
  NS_LOG_LOGIC ("receiver " << receiver << " origin " << rrepHeader.GetOrigin ());
  if (IsMyOwnAddress (rrepHeader.GetOrigin ()))
    {
      if (toDst.GetFlag () == IN_SEARCH)
        {
          m_routingTable.Update (newEntry);
          m_addressReqTimer[dst].Remove ();
          m_addressReqTimer.erase (dst);
        }

      // had ADD_DST_CASE_6 here first but thats pointless since we're the source of the RREQ, thus we already added the dst for sure to the QTable
      if (m_qlearner){
        NS_ASSERT(m_qlearner->CheckDestinationKnown(dst));
      }

      m_routingTable.LookupRoute (dst, toDst);
      SendPacketFromQueue (dst, toDst.GetRoute ());
      return;
    }

  RoutingTableEntry toOrigin;
  if (!m_routingTable.LookupRoute (rrepHeader.GetOrigin (), toOrigin) || toOrigin.GetFlag () == IN_SEARCH)
    {
      return; // Impossible! drop.
    }
  toOrigin.SetLifeTime (std::max (m_activeRouteTimeout, toOrigin.GetLifeTime ()));
  m_routingTable.Update (toOrigin);

  // Update information about precursors
  if (m_routingTable.LookupValidRoute (rrepHeader.GetDst (), toDst))
    {
      toDst.InsertPrecursor (toOrigin.GetNextHop ());
      m_routingTable.Update (toDst);

      RoutingTableEntry toNextHopToDst;
      m_routingTable.LookupRoute (toDst.GetNextHop (), toNextHopToDst);
      toNextHopToDst.InsertPrecursor (toOrigin.GetNextHop ());
      m_routingTable.Update (toNextHopToDst);

      toOrigin.InsertPrecursor (toDst.GetNextHop ());
      m_routingTable.Update (toOrigin);

      RoutingTableEntry toNextHopToOrigin;
      m_routingTable.LookupRoute (toOrigin.GetNextHop (), toNextHopToOrigin);
      toNextHopToOrigin.InsertPrecursor (toDst.GetNextHop ());
      m_routingTable.Update (toNextHopToOrigin);
    }
  SocketIpTtlTag tag;
  p->RemovePacketTag(tag);
  if (tag.GetTtl () < 2)
    {
      NS_LOG_DEBUG ("TTL exceeded. Drop RREP destination " << dst << " origin " << rrepHeader.GetOrigin ());
      return;
    }

  Ptr<Packet> packet = Create<Packet> ();
  SocketIpTtlTag ttl;
  ttl.SetTtl (tag.GetTtl() - 1);
  packet->AddPacketTag (ttl);
  packet->AddHeader (rrepHeader);
  TypeHeader tHeader (AODVTYPE_RREP);
  packet->AddHeader (tHeader);
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (toOrigin.GetInterface ());
  NS_ASSERT (socket);
  socket->SendTo (packet, 0, InetSocketAddress (toOrigin.GetNextHop (), AODV_PORT));
}

void
RoutingProtocol::RecvReplyAck (Ipv4Address neighbor)
{
  NS_LOG_FUNCTION (this);
  RoutingTableEntry rt;
  if(m_routingTable.LookupRoute (neighbor, rt))
    {
      rt.m_ackTimer.Cancel ();
      rt.SetFlag (VALID);
      m_routingTable.Update (rt);
    }
}

void
RoutingProtocol::ProcessHello (RrepHeader const & rrepHeader, Ipv4Address receiver )
{
  NS_LOG_FUNCTION (this << "from " << rrepHeader.GetDst ());

  if (m_qlearner) {
    // If a QLearner sees an AODV Hello, clearly, the node is a neighbour and it should be _SURE_ that that node is amongst the QLearner's available neighbours list
    // If that is not already the case, it should be added.
    m_qlearner->AddNeighbour( rrepHeader.GetDst() ); //GetDst seems like a strange thing to add but it's a Hello, so dst = the sender in that case
    // return; -- a lot of AODV traffic happens TODO if nodes revive ?
  }

  /*
   *  Whenever a node receives a Hello message from a neighbor, the node
   * SHOULD make sure that it has an active route to the neighbor, and
   * create one if necessary.
   */
  RoutingTableEntry toNeighbor;
  if (!m_routingTable.LookupRoute (rrepHeader.GetDst (), toNeighbor))
    {
      Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (receiver));
      RoutingTableEntry newEntry (/*device=*/ dev, /*dst=*/ rrepHeader.GetDst (), /*validSeqNo=*/ true, /*seqno=*/ rrepHeader.GetDstSeqno (),
                                              /*iface=*/ m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0),
                                              /*hop=*/ 1, /*nextHop=*/ rrepHeader.GetDst (), /*lifeTime=*/ rrepHeader.GetLifeTime ());
      m_routingTable.AddRoute (newEntry);

      /**
       * So if we receive a Hello on the AODV, it's basically a RREP with the destination field equal to the sender's IP
       * Receiving such a hello means the sender is a next-hop neighbour, as those are not forwarded either
       *
       * Setting the via field does not matter in this case, as it is a next-hop neighbour.
       *
       * ADD_DST_CASE_1
       */
      NS_LOG_DEBUG ("ADD_DST_CASE_1  ---  " << rrepHeader.GetDst() << "  " << rrepHeader.GetDst() );

      if (m_qlearner) {
        m_qlearner->AddDestination(rrepHeader.GetDst(), rrepHeader.GetDst(), Seconds(0));
      }

    }
  else
    {
      toNeighbor.SetLifeTime (std::max (Time (m_allowedHelloLoss * m_helloInterval), toNeighbor.GetLifeTime ()));
      toNeighbor.SetSeqNo (rrepHeader.GetDstSeqno ());
      toNeighbor.SetValidSeqNo (true);
      toNeighbor.SetFlag (VALID);
      toNeighbor.SetOutputDevice (m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (receiver)));
      toNeighbor.SetInterface (m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0));
      toNeighbor.SetHop (1);
      toNeighbor.SetNextHop (rrepHeader.GetDst ());
      m_routingTable.Update (toNeighbor);
    }
  if (m_enableHello)
    {
      m_nb.Update (rrepHeader.GetDst (), Time (m_allowedHelloLoss * m_helloInterval));
    }
}

void
RoutingProtocol::RecvError (Ptr<Packet> p, Ipv4Address src )
{
  NS_LOG_ERROR ("RERR received " << this << " from " << src);

  RerrHeader rerrHeader;
  p->RemoveHeader (rerrHeader);
  std::map<Ipv4Address, uint32_t> dstWithNextHopSrc;
  std::map<Ipv4Address, uint32_t> unreachable;
  m_routingTable.GetListOfDestinationWithNextHop (src, dstWithNextHopSrc);
  std::pair<Ipv4Address, uint32_t> un;
  while (rerrHeader.RemoveUnDestination (un))
    {
      for (std::map<Ipv4Address, uint32_t>::const_iterator i =
           dstWithNextHopSrc.begin (); i != dstWithNextHopSrc.end (); ++i)
      {
        if (i->first == un.first)
          {
            unreachable.insert (un);
          }
      }
    }

  std::vector<Ipv4Address> precursors;
  for (std::map<Ipv4Address, uint32_t>::const_iterator i = unreachable.begin ();
       i != unreachable.end ();)
    {
      if (!rerrHeader.AddUnDestination (i->first, i->second))
        {
          TypeHeader typeHeader (AODVTYPE_RERR);
          Ptr<Packet> packet = Create<Packet> ();
          SocketIpTtlTag tag;
          tag.SetTtl (1);
          packet->AddPacketTag (tag);
          packet->AddHeader (rerrHeader);
          packet->AddHeader (typeHeader);
          NS_LOG_DEBUG("called_1");
          SendRerrMessage (packet, precursors);
          rerrHeader.Clear ();
        }
      else
        {
          RoutingTableEntry toDst;
          m_routingTable.LookupRoute (i->first, toDst);
          toDst.GetPrecursors (precursors);
          ++i;
        }
    }
  if (rerrHeader.GetDestCount () != 0)
    {
      TypeHeader typeHeader (AODVTYPE_RERR);
      Ptr<Packet> packet = Create<Packet> ();
      SocketIpTtlTag tag;
      tag.SetTtl (1);
      packet->AddPacketTag (tag);
      packet->AddHeader (rerrHeader);
      packet->AddHeader (typeHeader);
      NS_LOG_DEBUG("called_2");
      SendRerrMessage (packet, precursors);
    }

  m_routingTable.InvalidateRoutesWithDst (unreachable);
}

void
RoutingProtocol::RouteRequestTimerExpire (Ipv4Address dst)
{
  NS_LOG_LOGIC (this);
  RoutingTableEntry toDst;
  if (m_routingTable.LookupValidRoute (dst, toDst))
    {
      SendPacketFromQueue (dst, toDst.GetRoute ());
      NS_LOG_LOGIC ("route to " << dst << " found");
      return;
    }
  /*
   *  If a route discovery has been attempted RreqRetries times at the maximum TTL without
   *  receiving any RREP, all data packets destined for the corresponding destination SHOULD be
   *  dropped from the buffer and a Destination Unreachable message SHOULD be delivered to the application.
   */
  if (toDst.GetRreqCnt () == m_rreqRetries)
    {
      NS_LOG_LOGIC ("route discovery to " << dst << " has been attempted RreqRetries (" << m_rreqRetries << ") times with ttl " << m_netDiameter);
      m_addressReqTimer.erase (dst);
      m_routingTable.DeleteRoute (dst);
      NS_LOG_DEBUG ("Route not found. Drop all packets with dst " << dst);
      m_queue.DropPacketWithDst (dst); //<deze>
      return;
    }

  if (toDst.GetFlag () == IN_SEARCH)
    {
      NS_LOG_DEBUG ("Resend RREQ to " << dst << " previous ttl " << toDst.GetHop ());
      SendRequest (dst);
    }
  else
    {
      NS_LOG_DEBUG ("Route down. Stop search. Drop packet with destination " << dst);
      m_addressReqTimer.erase (dst);
      m_routingTable.DeleteRoute (dst);
      m_queue.DropPacketWithDst (dst); //<deze>
    }
}

void
RoutingProtocol::HelloTimerExpire ()
{
  NS_LOG_FUNCTION (this);
  Time offset = Time (Seconds (0));
  if (m_lastBcastTime > Time (Seconds (0)))
    {
      offset = Simulator::Now () - m_lastBcastTime;
      NS_LOG_DEBUG ("Hello deferred due to last bcast at:" << m_lastBcastTime);
    }
  else
    {
      SendHello ();
    }
  m_htimer.Cancel ();
  Time diff = m_helloInterval - offset;
  m_htimer.Schedule (std::max (Time (Seconds (0)), diff));
  m_lastBcastTime = Time (Seconds (0));
}

void
RoutingProtocol::RreqRateLimitTimerExpire ()
{
  NS_LOG_FUNCTION (this);
  m_rreqCount = 0;
  m_rreqRateLimitTimer.Schedule (Seconds (1));
}

void
RoutingProtocol::RerrRateLimitTimerExpire ()
{
  NS_LOG_FUNCTION (this);
  m_rerrCount = 0;
  m_rerrRateLimitTimer.Schedule (Seconds (1));
}

void
RoutingProtocol::AckTimerExpire (Ipv4Address neighbor, Time blacklistTimeout)
{
  NS_LOG_FUNCTION (this);
  m_routingTable.MarkLinkAsUnidirectional (neighbor, blacklistTimeout);
}

void
RoutingProtocol::SendHello ()
{
  NS_LOG_FUNCTION (this);
  /* Broadcast a RREP with TTL = 1 with the RREP message fields set as follows:
   *   Destination IP Address         The node's IP address.
   *   Destination Sequence Number    The node's latest sequence number.
   *   Hop Count                      0
   *   Lifetime                       AllowedHelloLoss * HelloInterval
   */
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      RrepHeader helloHeader (/*prefix size=*/ 0, /*hops=*/ 0, /*dst=*/ iface.GetLocal (), /*dst seqno=*/ m_seqNo,
                                               /*origin=*/ iface.GetLocal (),/*lifetime=*/ Time (m_allowedHelloLoss * m_helloInterval));
      Ptr<Packet> packet = Create<Packet> ();
      SocketIpTtlTag tag;
      tag.SetTtl (1);
      packet->AddPacketTag (tag);
      packet->AddHeader (helloHeader);
      TypeHeader tHeader (AODVTYPE_RREP);
      packet->AddHeader (tHeader);
      // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
      Ipv4Address destination;
      if (iface.GetMask () == Ipv4Mask::GetOnes ())
        {
          destination = Ipv4Address ("255.255.255.255");
        }
      else
        {
          destination = iface.GetBroadcast ();
        }
      Time jitter = Time (MilliSeconds (m_uniformRandomVariable->GetInteger (0, 10)));
      Simulator::Schedule (jitter, &RoutingProtocol::SendTo, this , socket, packet, destination);
    }
}

void
RoutingProtocol::SendPacketFromQueue (Ipv4Address dst, Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this);

  QueueEntry queueEntry;
  while (m_queue.Dequeue (dst, queueEntry)) //<deze>
    {
      DeferredRouteOutputTag tag;
      Ptr<Packet> p = ConstCast<Packet> (queueEntry.GetPacket ());

      Ptr<Ipv4Route> route_to_use = Create<Ipv4Route>(*route);

      if (!m_qlearner) {
        NS_LOG_DEBUG( "Dequeue " << p->GetUid()  << " for " << dst << " at time " << Simulator::Now().As (Time::MS)
          << "  queue length right now: " << m_queue.GetSize() << ", max: " << m_queue.GetMaxQueueLen());
      }

      std::stringstream ss;  p->Print(ss);  NS_LOG_DEBUG(ss.str());

      if (m_qlearner) {

        NS_LOG_DEBUG( "Dequeue " << p->GetUid()  << " for " << dst << " at time " << Simulator::Now().As (Time::MS)
          << "  queue length right now: " << m_queue.GetSize() << ", max: " << m_queue.GetMaxQueueLen()
          << " time in queue was: " << m_qlearner->GetPacketTable()->GetPacketQueueTime(p->GetUid()) << std::endl);

        NS_ASSERT_MSG(queueEntry.GetIpv4Header().GetSource() == route->GetSource(), "These should be equal..." << route->GetSource() << " " << queueEntry.GetIpv4Header().GetSource());
        if (!m_qlearner->Route(route_to_use, p, dst, route->GetSource() ) ) {
          // checking pkt forwarding counting too many
          Icmpv4TimeExceeded ii;
          if (!CheckIcmpTTLExceeded(p,ii)){
            NS_FATAL_ERROR("I dont think this actually ever happens because if we have to send packets from queue we should already indeed also have a neighbour to use.");
          }
          //Drop it, no neighbours = no routes...
          return;
        }

        // this part added because packets leaving the queue are having queue time
        // in the travel time already, this is not what we want.
        QLrnInfoTag qtag, old_qtag;
        if (p->PeekPacketTag(old_qtag)) {
          qtag = old_qtag;
          qtag.SetTime(Simulator::Now().GetInteger());
        }
        p->ReplacePacketTag(qtag);

      }

      if (p->RemovePacketTag (tag) &&
          tag.GetInterface() != -1 &&
          tag.GetInterface() != m_ipv4->GetInterfaceForDevice (route->GetOutputDevice ()))
        {
          NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
          return;
        }
      UnicastForwardCallback ucb = queueEntry.GetUnicastForwardCallback ();
      Ipv4Header header = queueEntry.GetIpv4Header ();
      header.SetSource (route->GetSource ());
      header.SetTtl (header.GetTtl () + 1); // compensate extra TTL decrement by fake loopback routing
      ucb (route, p, header);
      // route->SetGateway(old_route.GetGateway()); //possible fix to issue of QLearner permanently overwriting routes --
      //  so this was the problem , route was being set back to what it was before, if we delay a bit this is immediately noticed
      // using route_to_use should fix it ?
    }
}

void
RoutingProtocol::SendRerrWhenBreaksLinkToNextHop (Ipv4Address nextHop)
{

  NS_LOG_FUNCTION (this << nextHop);
  RerrHeader rerrHeader;
  std::vector<Ipv4Address> precursors;
  std::map<Ipv4Address, uint32_t> unreachable;

  if (m_qlearner) {
    NS_LOG_UNCOND( nextHop << " is down at " << m_ipv4->GetAddress(1,0).GetLocal() << " " << Simulator::Now().As(Time::S)  <<"\n");
    NS_LOG_DEBUG("aodv down   " << nextHop << " is down at " << m_ipv4->GetAddress(1,0).GetLocal() << " " << Simulator::Now().As(Time::S) );
    m_qlearner->NotifyLinkDown(nextHop); //Trying this...
    return;
  }

  RoutingTableEntry toNextHop;
  if (!m_routingTable.LookupRoute (nextHop, toNextHop))
    return;

  //Get everyone that is upstream that would send packets making me use this nextHop
  toNextHop.GetPrecursors (precursors);
  rerrHeader.AddUnDestination (nextHop, toNextHop.GetSeqNo ());
  //Get all the unreachable addresses that would be used ?
  m_routingTable.GetListOfDestinationWithNextHop (nextHop, unreachable);
  for (std::map<Ipv4Address, uint32_t>::const_iterator i = unreachable.begin (); i
       != unreachable.end ();)
    {
      if (!rerrHeader.AddUnDestination (i->first, i->second))
        {
          NS_LOG_ERROR ("Send RERR message with maximum size.");
          TypeHeader typeHeader (AODVTYPE_RERR);
          Ptr<Packet> packet = Create<Packet> ();
          SocketIpTtlTag tag;
          tag.SetTtl (1);
          packet->AddPacketTag (tag);
          packet->AddHeader (rerrHeader);
          packet->AddHeader (typeHeader);
          NS_LOG_DEBUG("called_3");
          SendRerrMessage (packet, precursors);
          rerrHeader.Clear ();
        }
      else
        {
          RoutingTableEntry toDst;
          m_routingTable.LookupRoute (i->first, toDst);
          toDst.GetPrecursors (precursors);
          ++i;
        }
    }
  if (rerrHeader.GetDestCount () != 0)
    {
      NS_LOG_ERROR ("Send RERR message with maximum size.");
      TypeHeader typeHeader (AODVTYPE_RERR);
      Ptr<Packet> packet = Create<Packet> ();
      SocketIpTtlTag tag;
      tag.SetTtl (1);
      packet->AddPacketTag (tag);
      packet->AddHeader (rerrHeader);
      packet->AddHeader (typeHeader);
      SendRerrMessage (packet, precursors);
    }
  unreachable.insert (std::make_pair (nextHop, toNextHop.GetSeqNo ()));

  m_routingTable.InvalidateRoutesWithDst (unreachable);
}

void
RoutingProtocol::SendRerrWhenNoRouteToForward (Ipv4Address dst,
                                               uint32_t dstSeqNo, Ipv4Address origin)
{
  NS_LOG_FUNCTION (this);
  // A node SHOULD NOT originate more than RERR_RATELIMIT RERR messages per second.
  if (m_rerrCount == m_rerrRateLimit)
    {
      // Just make sure that the RerrRateLimit timer is running and will expire
      NS_ASSERT (m_rerrRateLimitTimer.IsRunning ());
      // discard the packet and return
      NS_LOG_LOGIC ("RerrRateLimit reached at " << Simulator::Now ().GetSeconds () << " with timer delay left "
                                                << m_rerrRateLimitTimer.GetDelayLeft ().GetSeconds ()
                                                << "; suppressing RERR");
      return;
    }
  RerrHeader rerrHeader;
  rerrHeader.AddUnDestination (dst, dstSeqNo);
  RoutingTableEntry toOrigin;
  Ptr<Packet> packet = Create<Packet> ();
  SocketIpTtlTag tag;
  tag.SetTtl (1);
  packet->AddPacketTag (tag);
  packet->AddHeader (rerrHeader);
  packet->AddHeader (TypeHeader (AODVTYPE_RERR));
  if (m_routingTable.LookupValidRoute (origin, toOrigin))
    {
      Ptr<Socket> socket = FindSocketWithInterfaceAddress (
          toOrigin.GetInterface ());
      NS_ASSERT (socket);
      NS_LOG_ERROR ("Unicast RERR to the source of the data transmission");
      socket->SendTo (packet, 0, InetSocketAddress (toOrigin.GetNextHop (), AODV_PORT));
    }
  else
    {
      for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator i =
             m_socketAddresses.begin (); i != m_socketAddresses.end (); ++i)
        {
          Ptr<Socket> socket = i->first;
          Ipv4InterfaceAddress iface = i->second;
          NS_ASSERT (socket);
          NS_LOG_ERROR ("Broadcast RERR message from interface " << iface.GetLocal ());
          // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
          Ipv4Address destination;
          if (iface.GetMask () == Ipv4Mask::GetOnes ())
            {
              destination = Ipv4Address ("255.255.255.255");
            }
          else
            {
              destination = iface.GetBroadcast ();
            }
          socket->SendTo (packet->Copy (), 0, InetSocketAddress (destination, AODV_PORT));
        }
    }
}

void
RoutingProtocol::SendRerrMessage (Ptr<Packet> packet, std::vector<Ipv4Address> precursors)
{
  NS_LOG_FUNCTION (this);

  if (precursors.empty ())
    {
      NS_LOG_LOGIC ("No precursors");
      return;
    }
  // A node SHOULD NOT originate more than RERR_RATELIMIT RERR messages per second.
  if (m_rerrCount == m_rerrRateLimit)
    {
      // Just make sure that the RerrRateLimit timer is running and will expire
      NS_ASSERT (m_rerrRateLimitTimer.IsRunning ());
      // discard the packet and return
      NS_LOG_LOGIC ("RerrRateLimit reached at " << Simulator::Now ().GetSeconds () << " with timer delay left "
                                                << m_rerrRateLimitTimer.GetDelayLeft ().GetSeconds ()
                                                << "; suppressing RERR");
      return;
    }
  // If there is only one precursor, RERR SHOULD be unicast toward that precursor
  if (precursors.size () == 1)
    {
      RoutingTableEntry toPrecursor;
      if (m_routingTable.LookupValidRoute (precursors.front (), toPrecursor))
        {
          Ptr<Socket> socket = FindSocketWithInterfaceAddress (toPrecursor.GetInterface ());
          NS_ASSERT (socket);
          NS_LOG_LOGIC ("one precursor => unicast RERR to " << toPrecursor.GetDestination () << " from " << toPrecursor.GetInterface ().GetLocal ());
          Simulator::Schedule (Time (MilliSeconds (m_uniformRandomVariable->GetInteger (0, 10))), &RoutingProtocol::SendTo, this, socket, packet, precursors.front ());
          m_rerrCount++;
        }
      return;
    }

  //  Should only transmit RERR on those interfaces which have precursor nodes for the broken route
  std::vector<Ipv4InterfaceAddress> ifaces;
  RoutingTableEntry toPrecursor;
  for (std::vector<Ipv4Address>::const_iterator i = precursors.begin (); i != precursors.end (); ++i)
    {
      if (m_routingTable.LookupValidRoute (*i, toPrecursor) &&
          std::find (ifaces.begin (), ifaces.end (), toPrecursor.GetInterface ()) == ifaces.end ())
        {
          ifaces.push_back (toPrecursor.GetInterface ());
        }
    }

  for (std::vector<Ipv4InterfaceAddress>::const_iterator i = ifaces.begin (); i != ifaces.end (); ++i)
    {
      Ptr<Socket> socket = FindSocketWithInterfaceAddress (*i);
      NS_ASSERT (socket);
      NS_LOG_LOGIC ("Broadcast RERR message from interface " << i->GetLocal ());
      // std::cout << "Broadcast RERR message from interface " << i->GetLocal () << std::endl;
      // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
      Ptr<Packet> p = packet->Copy ();
      Ipv4Address destination;
      if (i->GetMask () == Ipv4Mask::GetOnes ())
        {
          destination = Ipv4Address ("255.255.255.255");
        }
      else
        {
          destination = i->GetBroadcast ();
        }
      Simulator::Schedule (Time (MilliSeconds (m_uniformRandomVariable->GetInteger (0, 10))), &RoutingProtocol::SendTo, this, socket, p, destination);
    }
}

Ptr<Socket>
RoutingProtocol::FindSocketWithInterfaceAddress (Ipv4InterfaceAddress addr ) const
{
  NS_LOG_FUNCTION (this << addr);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      if (iface == addr)
        return socket;
    }
  Ptr<Socket> socket;
  return socket;
}

Ptr<Socket>
RoutingProtocol::FindSubnetBroadcastSocketWithInterfaceAddress (Ipv4InterfaceAddress addr ) const
{
  NS_LOG_FUNCTION (this << addr);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketSubnetBroadcastAddresses.begin (); j != m_socketSubnetBroadcastAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      if (iface == addr)
        return socket;
    }
  Ptr<Socket> socket;
  return socket;
}

void
RoutingProtocol::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  uint32_t startTime;
  if (m_enableHello)
    {
      m_htimer.SetFunction (&RoutingProtocol::HelloTimerExpire, this);
      startTime = m_uniformRandomVariable->GetInteger (0, 100);
      NS_LOG_DEBUG ("Starting at time " << startTime << "ms");
      m_htimer.Schedule (MilliSeconds (startTime));
    }
  Ipv4RoutingProtocol::DoInitialize ();
}

} //namespace aodv
} //namespace ns3
