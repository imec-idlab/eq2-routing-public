/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: George F. Riley<riley@ece.gatech.edu>
//

// ns3 - On/Off Data Source Application class
// George F. Riley, Georgia Tech, Spring 2007
// Adapted from ApplicationOnOff in GTNetS.

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "onoff-application.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"

#include "ns3/thomas-configuration.h"
#include "ns3/thomas-packet-tags.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OnOffApplication");

NS_OBJECT_ENSURE_REGISTERED (OnOffApplication);

TypeId
OnOffApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OnOffApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<OnOffApplication> ()
    .AddAttribute ("DataRate", "The data rate in on state.",
                   DataRateValue (DataRate ("500kb/s")),
                   MakeDataRateAccessor (&OnOffApplication::m_cbrRate),
                   MakeDataRateChecker ())
    .AddAttribute ("PacketSize", "The size of packets sent in on state",
                   UintegerValue (512),
                   MakeUintegerAccessor (&OnOffApplication::m_pktSize),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&OnOffApplication::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("OnTime", "A RandomVariableStream used to pick the duration of the 'On' state.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                   MakePointerAccessor (&OnOffApplication::m_onTime),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("OffTime", "A RandomVariableStream used to pick the duration of the 'Off' state.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                   MakePointerAccessor (&OnOffApplication::m_offTime),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("MaxBytes",
                   "The total number of bytes to send. Once these bytes are sent, "
                   "no packet is sent again, even in on state. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&OnOffApplication::m_maxBytes),
                   MakeUintegerChecker<uint64_t> ())
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&OnOffApplication::m_tid),
                   MakeTypeIdChecker ())
    .AddAttribute ("Learning", "is it a learning traffic gen or not",
                   BooleanValue (true),
                   MakeBooleanAccessor (&OnOffApplication::m_learning),
                   MakeBooleanChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&OnOffApplication::m_txTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}


OnOffApplication::OnOffApplication ()
  : m_socket (0),
    m_connected (false),
    m_residualBits (0),
    m_lastStartTime (Seconds (0)),
    m_backup_maxBytes(0),
    m_totBytes (0),
    m_learning(false)
{
  NS_LOG_FUNCTION (this);
  m_packet_number = 0;
}

OnOffApplication::~OnOffApplication()
{
  NS_LOG_FUNCTION (this);
}

void
OnOffApplication::SetMaxBytes (uint64_t maxBytes)
{
  // std::cout << "setting max bytes at " << Simulator::Now().GetSeconds() << " on node " << GetNode()->GetId() << "  from "
  //           << m_maxBytes << "  to  " << maxBytes << " on a " << (m_learning ? " learning " : " non learning ") << " onoff appl." << std::endl;
  if (m_socket == 0 && m_maxBytes == FREEZE_ONOFFAPPLICATION_SENDING) {
    // std::cout << "setting max bytes  & stopping it here b/c appl not yet started!" << std::endl;
    return;
  }

  if (m_maxBytes != 0 && m_maxBytes != FREEZE_ONOFFAPPLICATION_SENDING) { m_backup_maxBytes = m_maxBytes; } // save the value of maxBytes so that we can put it back later
  if (m_backup_maxBytes != 0 && maxBytes == 0 ) {
    NS_ASSERT(maxBytes != FREEZE_ONOFFAPPLICATION_SENDING);
    m_maxBytes = m_backup_maxBytes;
  } else {
    m_maxBytes = maxBytes;
  }


  if (maxBytes != FREEZE_ONOFFAPPLICATION_SENDING) { CancelEvents(); ScheduleNextTx(); } //hacking... - need that CancelEvents() call though
  else {
    NS_LOG_DEBUG("STOPPING amount of "<< (m_learning?"learning":"data")<< " traffic from " << m_cbrRate << " to " << 0 << " at time " << Simulator::Now().GetSeconds() );
    CancelEvents ();
  } // more hacking : if a sendEvent is set, learning stops, then starts again before the sendEvent goes off we get messy timings and errors!
}

void
OnOffApplication::ReduceAmountOfTraffic(uint64_t new_data_rate, float reduce_by_percentage)  {
  auto old_cbrRate = m_cbrRate;
  if (m_cbrRate <  DataRate("6000bps")) {
    //TODO - dont make learning TOO slow ?
    return;
  }
  if (new_data_rate == 0 && reduce_by_percentage == 0.0) {
    m_cbrRate = DataRate(m_cbrRate.GetBitRate()/10);
  } else if (new_data_rate > 0) {
    m_cbrRate = DataRate(new_data_rate);
  } else if (reduce_by_percentage > 0.0) {
    NS_ASSERT_MSG(reduce_by_percentage < 1, "trying to increase learning traffic instead of reducing it");
    m_cbrRate = DataRate(m_cbrRate.GetBitRate() * reduce_by_percentage);
  } else {
    NS_FATAL_ERROR("In a call to OnOffApplication::ReduceAmountOfTraffic parameters were set incorrectly. " << new_data_rate << " " << reduce_by_percentage);
  }

  NS_LOG_UNCOND("REDUCING amount of "<< (m_learning?"learning":"data")<< " traffic from " << old_cbrRate
        << " to " << m_cbrRate << " at time " << Simulator::Now().GetSeconds()
        << " and at node " << GetNode()->GetId() ) ;
  CancelEvents();
  m_cbrRateFailSafe = m_cbrRate; //TODO check the impact of these three calls
  ScheduleNextTx();
}

void
OnOffApplication::IncreaseAmountOfTraffic(uint64_t new_data_rate, float increase_by_percentage)  {
  auto old_cbrRate = m_cbrRate;
  if (m_cbrRate >  DataRate("90000bps")) { //to stop flooding the network, causes nodes to fail
    //TODO - dont make learning TOO fast ?
    return;
  }
  CancelEvents(); // Due to issue in test96, this needs to be here. Or will have a huge amt of residual bits (underflow)
  if (new_data_rate == 0 && increase_by_percentage == 0.0) {
    m_cbrRate = DataRate(m_cbrRate.GetBitRate()*10);
  } else if (new_data_rate > 0) {
    m_cbrRate = DataRate(new_data_rate);
  } else if (increase_by_percentage > 0.0) {
    NS_ASSERT_MSG(increase_by_percentage > 1, "trying to reduce learning traffic instead of increasing it");
    m_cbrRate = DataRate(m_cbrRate.GetBitRate() * increase_by_percentage);
  } else {
    NS_FATAL_ERROR("In a call to OnOffApplication::IncreaseAmountOfTraffic parameters were set incorrectly. " << new_data_rate << " " << increase_by_percentage);
  }

  if (m_cbrRate < DataRate("1000bps")) {
    m_cbrRate = DataRate("1000bps");
  }

  NS_LOG_UNCOND("INCREASING amount of "<< (m_learning?"learning":"data")<< " traffic from "
      << old_cbrRate << " to " << m_cbrRate << " at time " << Simulator::Now().GetSeconds()
      << " and at node " << GetNode()->GetId() ) ;

  m_cbrRateFailSafe = m_cbrRate;
  ScheduleNextTx();
}

void
OnOffApplication::SetLearning (bool b)
{
  m_learning = b;
}

Ptr<Socket>
OnOffApplication::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

int64_t
OnOffApplication::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_onTime->SetStream (stream);
  m_offTime->SetStream (stream + 1);
  return 2;
}

void
OnOffApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

// Application Methods
void OnOffApplication::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);

  if (m_maxBytes == FREEZE_ONOFFAPPLICATION_SENDING && m_learning /* temp, just to be sure its the right one */) {
    // NS_ASSERT(GetNode()->GetId() == 2); -- now that we're expanding to more nodes, not only 2 is observed
    m_maxBytes = 0;
  }

  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      if (Inet6SocketAddress::IsMatchingType (m_peer))
        {
          m_socket->Bind6 ();
        }
      else if (InetSocketAddress::IsMatchingType (m_peer) ||
               PacketSocketAddress::IsMatchingType (m_peer))
        {
          m_socket->Bind ();
        }
      m_socket->Connect (m_peer);
      m_socket->SetAllowBroadcast (true);
      m_socket->ShutdownRecv ();

      m_socket->SetConnectCallback (
        MakeCallback (&OnOffApplication::ConnectionSucceeded, this),
        MakeCallback (&OnOffApplication::ConnectionFailed, this));
    }
  m_cbrRateFailSafe = m_cbrRate;

  // Insure no pending event
  CancelEvents ();
  // If we are not yet connected, there is nothing to do here
  // The ConnectionComplete upcall will start timers at that time
  //if (!m_connected) return;
  ScheduleStartEvent ();
}

void OnOffApplication::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  CancelEvents ();
  if(m_socket != 0)
    {
      m_socket->Close ();
    }
  else
    {
      NS_LOG_WARN ("OnOffApplication found null socket to close in StopApplication");
    }
}

void OnOffApplication::CancelEvents ()
{
  NS_LOG_FUNCTION (this);

  if (m_sendEvent.IsRunning () && m_cbrRateFailSafe == m_cbrRate )
    { // Cancel the pending send packet event
      // Calculate residual bits since last packet sent
      Time delta (Simulator::Now () - m_lastStartTime);
      int64x64_t bits = delta.To (Time::S) * m_cbrRate.GetBitRate ();
      m_residualBits += bits.GetHigh ();
    }
  m_cbrRateFailSafe = m_cbrRate;
  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_startStopEvent);
}

// Event handlers
void OnOffApplication::StartSending ()
{
  NS_LOG_FUNCTION (this);
  m_lastStartTime = Simulator::Now ();
  ScheduleNextTx ();  // Schedule the send packet event
  ScheduleStopEvent ();
}

void OnOffApplication::StopSending ()
{
  NS_LOG_FUNCTION (this);
  CancelEvents ();

  ScheduleStartEvent ();
}

// Private helpers
void OnOffApplication::ScheduleNextTx ()
{
  NS_LOG_FUNCTION (this);
  if (m_maxBytes != FREEZE_ONOFFAPPLICATION_SENDING && (m_maxBytes == 0 || m_totBytes < m_maxBytes))
    {
      uint32_t bits = m_pktSize * 8 - m_residualBits;
      NS_LOG_DEBUG("bits = " << bits); // this bits becomes huge due to m_residualBits
      Time nextTime (Seconds (bits /
                              static_cast<double>(m_cbrRate.GetBitRate ()))); // Time till next packet
      NS_LOG_LOGIC("nextTime = " << nextTime.As(Time::MS));
      m_sendEvent = Simulator::Schedule (nextTime,
                                         &OnOffApplication::SendPacket, this);
    }
  else if (m_maxBytes == FREEZE_ONOFFAPPLICATION_SENDING) { } // so it wont stop automatically.. then we can re-enable it when we need to learn more
  else
    { // All done, cancel any pending events
      StopApplication ();
    }
}

void OnOffApplication::ScheduleStartEvent ()
{  // Schedules the event to start sending data (switch to the "On" state)
  NS_LOG_FUNCTION (this);

  Time offInterval = Seconds (m_offTime->GetValue ());
  NS_LOG_LOGIC ("start at " << offInterval);
  m_startStopEvent = Simulator::Schedule (offInterval, &OnOffApplication::StartSending, this);
}

void OnOffApplication::ScheduleStopEvent ()
{  // Schedules the event to stop sending data (switch to "Off" state)
  NS_LOG_FUNCTION (this);

  Time onInterval = Seconds (m_onTime->GetValue ());
  NS_LOG_LOGIC ("stop at " << onInterval);
  m_startStopEvent = Simulator::Schedule (onInterval, &OnOffApplication::StopSending, this);
}


void OnOffApplication::SendPacket ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG( m_sendEvent.GetTs() << " now: " << Simulator::Now().GetInteger() << " " << (int(m_sendEvent.GetTs()) == int(Simulator::Now().GetInteger())) );
  NS_ASSERT (m_sendEvent.IsExpired ());
  Ptr<Packet> packet = Create<Packet> (m_pktSize);
  m_txTrace (packet);
  PortNrTag pnt(InetSocketAddress::ConvertFrom (m_peer).GetPort (), m_learning, (!m_learning?m_packet_number:0));
  m_packet_number += 1;
  packet->AddPacketTag(pnt);
  m_socket->Send (packet);
  m_totBytes += m_pktSize;
  if (InetSocketAddress::IsMatchingType (m_peer))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                   << "s on-off application sent "
                   <<  packet->GetSize () << " bytes to "
                   << InetSocketAddress::ConvertFrom(m_peer).GetIpv4 ()
                   << " port " << InetSocketAddress::ConvertFrom (m_peer).GetPort ()
                   << " total Tx " << m_totBytes << " bytes");
    }
  else if (Inet6SocketAddress::IsMatchingType (m_peer))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                   << "s on-off application sent "
                   <<  packet->GetSize () << " bytes to "
                   << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6 ()
                   << " port " << Inet6SocketAddress::ConvertFrom (m_peer).GetPort ()
                   << " total Tx " << m_totBytes << " bytes");
    }
  m_lastStartTime = Simulator::Now ();
  m_residualBits = 0;
  ScheduleNextTx ();
}


void OnOffApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void OnOffApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}


} // Namespace ns3
