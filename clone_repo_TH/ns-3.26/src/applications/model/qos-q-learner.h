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

#ifndef QOS_Q_LEARNER_H
#define QOS_Q_LEARNER_H

#include "ns3/node-container.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/ptr.h"
#include "ns3/qlrn-header.h"
#include "ns3/qos-qlrn-header.h"
#include "ns3/thomas-packet-tags.h"
#include "ns3/qtable.h"
#include "ns3/packettable.h"
#include "q-learner.h"
#include <cstdlib>

namespace ns3 {

class QoSQLearner : public QLearner {
public:
  QoSQLearner(float,float,float);
  static TypeId GetTypeId (void);
  ~QoSQLearner() { } ;
  virtual bool CheckQLrnHeader(Ptr<const Packet> p, QoSQLrnHeader& qlrnHeader);
  virtual bool CheckQLrnHeader(Ptr<const Packet> p, QLrnHeader& qlrnHeader){ return false; }
  virtual bool CheckQLrnHeader_withoutUDP(Ptr<const Packet> p, QoSQLrnHeader& qlrnHeader);
  virtual bool CheckQLrnHeader_withoutUDP(Ptr<const Packet> p, QLrnHeader& qlrnHeader) { return false; }
  virtual void UpdateAvgDelay(PacketTimeSentTag,PortNrTag);
  void ApplyMetricsToQValue(Ipv4Address, Ipv4Address, uint64_t,TrafficType, uint64_t, uint64_t, float);

private:
  virtual void Receive (Ptr<Socket> socket);
  virtual void Send(Ipv4Address node_to_notify, uint64_t packet_Uid, Time travel_time, Ipv4Address packet_dst, Ipv4Address packet_src, TrafficType t, bool sender_converged) ;
  virtual void StartApplication(void);
  uint64_t m_q_value_when_all_blacklisted;
  //
};

} //namespace ns3

#endif /* QOS_Q_LEARNER_H */
