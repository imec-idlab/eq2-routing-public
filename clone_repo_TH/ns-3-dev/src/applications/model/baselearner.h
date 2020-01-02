/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */

#ifndef BASELEARNER_H
#define BASELEARNER_H

#include "ns3/wifi-mac-queue.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/arp-l3-protocol.h"
#include "ns3/onoff-application.h"
#include "ns3/node-container.h"
#include "ns3/icmpv4.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/aodv-routing-protocol.h"
#include "ns3/aodv-rtable.h"
#include "ns3/aodv-packet.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/udp-header.h"
#include "ns3/application-container.h"
#include "ns3/tcp-header.h"
#include "ns3/qlrn-header.h"
#include "ns3/qos-qlrn-header.h"
#include "ns3/thomas-packet-tags.h"
#include "ns3/traffic-types.h"
#include "ns3/packettable.h"
#include "ns3/mobility-module.h" /* makes STA mobile but we dont want any of that <-- needed for placement in grid */
#include "ns3/thomas-configuration.h"
#include "ns3/qdecision.h"
#include <iomanip>
#include <iostream>

// not using v6 anyway
// #include "ns3/ipv6-address.h"
// #include "ns3/inet6-socket-address.h"

namespace ns3 {

int traffic_string_to_port_number (std::string traffic);
TrafficType traffic_string_to_traffic_type(std::string traffic);
std::string traffic_type_to_traffic_string(TrafficType t);

class Socket;
class Packet;

/**
 * \ingroup qlearner
 * \brief A Q Learner client
 *
 * Every packet sent should be returned by the server and received here.
 */
class BaseLearner : public Application
{
public:
  static const uint32_t QLRN_PORT = 404;;
  // TODO HANS - FIX THIS FIRST
  static TypeId GetTypeId (void){return TypeId ("ns3::BaseLearner");};

  //BaseLearner (){};
  //BaseLearner (float,float,float);
  virtual ~BaseLearner (){};

  PacketTable* GetPacketTable() {return &m_packet_info;}

  virtual void Send(Ipv4Address node_to_notify, uint64_t packet_Uid, Time timestamp_of_arrival, Ipv4Address packet_dst, Ipv4Address packet_src, TrafficType t, bool sender_converged) = 0;

  virtual bool CheckQLrnHeader(Ptr<const Packet> p, QLrnHeader& qlrnHeader) = 0;
  virtual bool CheckQLrnHeader_withoutUDP(Ptr<const Packet> p, QLrnHeader& qlrnHeader) = 0;
  virtual bool CheckQLrnHeader(Ptr<const Packet> p, QoSQLrnHeader& qlrnHeader) = 0;
  virtual bool CheckQLrnHeader_withoutUDP(Ptr<const Packet> p, QoSQLrnHeader& qlrnHeader) = 0;

  virtual bool AddDestination (Ipv4Address via, Ipv4Address dst, Time t) = 0;
  virtual bool Route ( Ptr<Ipv4Route>, Ptr<Packet>, const Ipv4Header&) = 0;
  virtual bool Route ( Ptr<Ipv4Route>, Ptr<Packet>, const Ipv4Address&, const Ipv4Address&) =0;
  virtual void RouteDiffBasedOnType (QDecisionEntry*, Ipv4Address, Ipv4Address, TrafficType) = 0;
  virtual bool CheckDestinationKnown(const Ipv4Address& i) = 0;
  virtual void HandleRouteOutput(Ptr<Packet>, const Ipv4Header&, TrafficType) = 0;
  virtual void HandleRouteInput (Ptr<Packet>, const Ipv4Header&, bool, bool&, TrafficType) = 0;
  virtual Ipv4Address GetNextHop(Ipv4Address dst, TrafficType t) = 0;
  virtual void ChangeQValuesFromZero(Ipv4Address dst, Ipv4Address aodv_next_hop) = 0;

  /// if true, print qtables to files, if false, dont
  bool m_print_qtables;
  virtual std::string PrintQTable(TrafficType t) = 0;
  virtual void FinaliseQTables(TrafficType t) = 0;

  virtual void FixRoute(Ptr<Ipv4Route> route,  Ptr<NetDevice> net, Ipv4Address src) = 0;
  virtual void ARPDeadTrace(  Ipv4Address ) = 0;
  virtual void MACEnqueuePacket (uint64_t,bool=false) = 0;
  virtual void MACDequeuePacket (uint64_t,bool=false) = 0;
  virtual void NotifyLinkDown(Ipv4Address) = 0;
  virtual void AddNeighbour(Ipv4Address n) = 0;

  int QStatistics() { return m_control_packets_sent; }
  virtual std::string GetStatistics() = 0;

  Ptr<aodv::RoutingProtocol> GetAODV() { return aodvProto; }
  virtual bool SetOtherQLearners(NodeContainer) = 0;

  virtual void RestartAfterMoving() = 0;

  virtual QDecision* GetQTable(TrafficType t) = 0;

  virtual void InitializeLearningPhases(std::vector<Ipv4Address>) = 0;
  void SetLearningPhase(bool b, Ipv4Address i) { m_learning_phase[i] = b; }
  bool IsInLearningPhase(Ipv4Address i) { return m_learning_phase[i] && m_use_learning_phases; }
  void SetDelay(int new_delay) { m_delay = new_delay; }
  void SetJittering(bool b) { m_jittery_qlearner = b; }
  void SetSlow(bool b) { m_slow_qlearner = b; }
  void SetSmallLrnTraffic (bool b) { m_small_learning_stream = b;}

  // ApplicationContainer* learning_traffic_applications;
  // ApplicationContainer* real_traffic;
  Ptr<Application> learning_traffic_applications;
  Ptr<Application> real_traffic;
  virtual void SetLearningTrafficGen( Ptr<Application> a ) = 0;
  virtual void SetTrafficGen( Ptr<Application> a ) = 0;

  virtual void UpdateAvgDelay(PacketTimeSentTag,PortNrTag) = 0;
  float AvgDelayAsFloat() { return m_running_avg_latency.second; }

  bool IamAmongTheDestinations() { return std::find(m_traffic_destinations.begin(), m_traffic_destinations.end(), m_this_node_ip ) != m_traffic_destinations.end(); }
  void SetTrafficSources(std::vector<Ipv4Address> i) { m_traffic_sources = i;}
  void SetTrafficDestinations(std::vector<Ipv4Address> i) { m_traffic_destinations = i;}
  void SetMyTrafficDst(Ipv4Address i) { m_my_sent_traffic_destination = i;}
  virtual std::vector<std::pair< std::pair<Ipv4Address,Ipv4Address>, float > > GetPacketLossPerNeighb() = 0 ;

  bool AllNeighboursBlacklisted(Ipv4Address i, TrafficType t) { return GetQTable(t)->AllNeighboursBlacklisted(i); }
protected:
  BaseLearner (){};
  void StopLearningStartTraffic(Ipv4Address);
  void StopTrafficStartLearning(Ipv4Address);
  virtual void DoDispose (void) = 0;
  virtual void StartApplication (void) = 0;
private:
  virtual void StopApplication (void) = 0;

  virtual void RouteHelper(Ptr<Ipv4Route>, Ipv4Address, TrafficType, uint64_t&, int, Ptr<Packet>) = 0;


  virtual std::vector<Ipv4Address> FindNeighboursManual (void) = 0;
  virtual std::vector<Ipv4Address> FindNeighboursAODV (void) = 0;
  virtual bool CheckAODVHeader(Ptr<const Packet> p) = 0;
  virtual void Receive (Ptr<Socket> socket) = 0;
  virtual void ReceiveAodv (Ptr<Socket> socket) = 0;

protected:
  uint32_t m_sent; //!< Counter for sent packets
  Ptr<Socket> m_socket; //!< Socket for potentially intercepting AODV traffic
  Ptr<Socket> m_qlrn_socket; // Socket for QLRN traffic

  /// Callbacks for tracing the packet Tx events
  TracedCallback<Ptr<const Packet> > m_txTrace;

  /// Underlying routing protocol
  Ptr<aodv::RoutingProtocol> aodvProto;

  /// std::list of ipv4 addresses of neighbours
  std::vector<Ipv4Address> neighbours;

  /// QTable
  QDecision* m_qtable;
  QDecision* m_qtable_video;
  QDecision* m_qtable_voip;

  ///
  PacketTable m_packet_info;

  ///
  bool m_verbose;

  ///
  bool m_in_test;

  /// since size() > 0 is no longer usable (since we're doing the E2E feedback thing with m_other_qlearners now) this bool is introduced
  bool m_ideal;

  ///
  float m_learningrate;

  /// value that epsilon needs to be lower than to make exploratory action
  float m_eps_thresh;
  Ptr<UniformRandomVariable> m_epsilon;

  /// discount factor for qlearning
  float m_gamma;

  /// for logging purposes, store this
  std::string m_name;

  /// Two booleans, one to enable the fake jittering, one to have it go up/down/up etc...
  bool m_jittery_qlearner;
  bool m_apply_jitter;
  /// and one boolean to make the alternative path slow, just not subject to jitter , this shows difference between path1 and path2
  bool m_slow_qlearner;
  /// and an int to store how many ms the delay should be
  int m_delay;

  /// Statistics
  virtual void QLrnPacketTracking(Ptr<const Packet> p) = 0;
  unsigned int m_control_packets_sent;
  int m_num_applications;
  std::set<unsigned int> tmp;
  std::map<Ipv4Address, Ptr<BaseLearner> > m_other_qlearners;

  std::vector<Ipv4Address> m_traffic_sources;
  std::vector<Ipv4Address> m_traffic_destinations;
  Ipv4Address m_my_sent_traffic_destination;

  /// boolean to keep track of wether or not we are using this learning phase system at all
  bool m_use_learning_phases;

  /// boolean to keep track of us being in (true) or out (false) of a learning phase
  std::map<Ipv4Address, bool> m_learning_phase;

  /// Value that, when the % difference between the expected time and the real time is exceeding it, a learning phase will be triggered
  float m_learning_threshold;

  bool m_output_data_to_file;
  virtual void OutputDataToFile(PacketTimeSentTag, Ptr<const Packet> p, bool learning_pkt,TrafficType t, Ipv4Address i) = 0;
  Ptr<OutputStreamWrapper> m_output_filestream;

  bool m_report_dst_to_src;

  // variables used to test things if needed, but not as important as to say that they will be part of the cli
  bool m_small_learning_stream;
  std::pair<int,float> m_running_avg_latency;

  Ipv4Address m_this_node_ip; //fine b/c we have 1 interface per node, not doing more
  float m_qconvergence_threshold;
  float m_rho; //for intermediate nodes, how likely they are to optimally forward a learning packet
  int m_max_retry;
  std::map<Ipv4Address,uint64_t> m_prev_delay_per_prev_hop;
  uint64_t m_prev_delay;
  uint16_t m_prev_loss;

  virtual void RoutingPacketViaNeighbToDst(Ipv4Address via,Ipv4Address dst) = 0;
  virtual uint64_t GetNumPktsSentViaNeighbToDst(uint64_t time,Ipv4Address via,Ipv4Address dst) = 0;
  std::map<std::pair<Ipv4Address,Ipv4Address>,std::vector<std::tuple<uint64_t,uint64_t,float> > > m_packet_sent_stats_per_neighb_per_time;
  virtual void CleanTheNumPktsTimestampMap(uint64_t time,Ipv4Address via,Ipv4Address dst) = 0; // remove all timestamps that are higher than currtime - 30s or so ?
  virtual void ReceivedPktFromPrevHopToDst(Ipv4Address prev_hop, Ipv4Address dst) = 0;
  virtual uint64_t GetNumRecvPktFromPrevHopToDst(Ipv4Address prev_hop, Ipv4Address dst) = 0;
  std::map<std::pair<Ipv4Address,Ipv4Address> ,std::pair<uint64_t,float> > m_packet_recv_stats_per_neighb;
  virtual void SetRealLossForOutput(Ipv4Address prev_hop, Ipv4Address dst, float) = 0;

  virtual Time TravelTimeHelper(QLrnInfoTag t) = 0;
  std::map<Ipv4Address,Time> m_backup_per_prev_hop_for_unusable_delay;
};

} // namespace ns3

#endif /* BASELEARNER_H */
