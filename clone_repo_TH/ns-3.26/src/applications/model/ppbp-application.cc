/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Doreid AMMAR - INRIA RESO team, LIP laboratory
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
 * Author: Doreid Ammar <doreid.ammar@gmail.com>
 */

#include "ppbp-application.h"
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/double.h"
#include "ns3/pointer.h"

NS_LOG_COMPONENT_DEFINE ("PPBPApplication");

using namespace std;

namespace ns3 {

	NS_OBJECT_ENSURE_REGISTERED (PPBPApplication);

	TypeId
	PPBPApplication::GetTypeId (void)
	{
		static TypeId tid = TypeId ("ns3::PPBPApplication")
		.SetParent<Application> ()
		.AddConstructor<PPBPApplication> ()
		.AddAttribute ("BurstIntensity", "The data rate of each burst.",
					   DataRateValue (DataRate ("1Mb/s")),
					   MakeDataRateAccessor (&PPBPApplication::m_cbrRate),
					   MakeDataRateChecker ())
		.AddAttribute ("PacketSize", "The size of packets sent in on state",
					   UintegerValue (1470),
					   MakeUintegerAccessor (&PPBPApplication::m_pktSize),
					   MakeUintegerChecker<uint32_t> (1))
		.AddAttribute ("MeanBurstArrivals", "Mean Active Sources",
					   StringValue ("ns3::ConstantRandomVariable[Constant=20.0]"),
						 MakePointerAccessor (&PPBPApplication::m_burstArrivals),
						 MakePointerChecker <RandomVariableStream>())
	  .AddAttribute ("MeanBurstTimeLength", "Pareto distributed burst durations",
 						 StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"),
		 				 MakePointerAccessor (&PPBPApplication::m_burstLength),
			 			 MakePointerChecker <RandomVariableStream>())
		.AddAttribute ("H", "Hurst parameter",
					   DoubleValue (0.7),
					   MakeDoubleAccessor (&PPBPApplication::m_h),
					   MakeDoubleChecker<double> ())
		.AddAttribute ("Remote", "The address of the destination",
					   AddressValue (),
					   MakeAddressAccessor (&PPBPApplication::m_peer),
					   MakeAddressChecker ())
		.AddAttribute ("Protocol", "The type of protocol to use.",
					   TypeIdValue (UdpSocketFactory::GetTypeId ()),
					   MakeTypeIdAccessor (&PPBPApplication::m_protocolTid),
					   MakeTypeIdChecker ())
		.AddTraceSource ("Tx", "A new packet is created and is sent",
						 MakeTraceSourceAccessor (&PPBPApplication::m_txTrace),
						 "ns3::Packet::TracedCallback" )
		;
		return tid;
	}

	PPBPApplication::PPBPApplication ()
	{
		NS_LOG_FUNCTION_NOARGS ();
		m_socket = 0;
		m_connected = false;
		m_lastStartTime = Seconds (0);
		m_totalBytes = 0;
		m_activebursts = 0;
		m_offPeriod = true;
	}

	PPBPApplication::~PPBPApplication()
	{
		NS_LOG_FUNCTION_NOARGS ();
	}

	uint32_t
	PPBPApplication::GetTotalBytes() const
	{
		return m_totalBytes;
	}

	void
	PPBPApplication::DoDispose (void)
	{
		NS_LOG_FUNCTION_NOARGS ();

		m_socket = 0;
		// chain up
		Application::DoDispose ();
	}

	// Application Methods
	void
	PPBPApplication::StartApplication() // Called at time specified by Start
	{
		NS_LOG_FUNCTION_NOARGS ();

		// Create the socket if not already
		if (!m_socket)
		{
			m_socket = Socket::CreateSocket (GetNode(), m_protocolTid);
			m_socket->Bind ();
			m_socket->Connect (m_peer);
		}
		// Insure no pending event
		CancelEvents ();
		ScheduleStartEvent();
	}

	void
	PPBPApplication::PPBP() // Poisson Pareto Burst
	{
		NS_LOG_FUNCTION_NOARGS ();

		double inter_burst_intervals;



		inter_burst_intervals = 1/m_burstArrivals->GetValue();

		Ptr<ExponentialRandomVariable> exp_var = CreateObject<ExponentialRandomVariable> ();
		exp_var->SetAttribute ("Mean", DoubleValue (inter_burst_intervals));

		Time t_poisson_arrival = Seconds (exp_var->GetValue());
		m_PoissonArrival = Simulator::Schedule(t_poisson_arrival,&PPBPApplication::PoissonArrival, this);

		// Pareto
		m_shape = 3 - 2 * m_h;
		m_timeSlot = Seconds((double) (m_shape - 1) * m_burstLength->GetValue() / m_shape);

		Ptr<ParetoRandomVariable> par_var = CreateObject<ParetoRandomVariable> ();

		// ParetoVariable pareto(m_burstLength.GetValue(), m_shape);
		//2 parameters means m, the mean and s the shape!
		par_var->SetAttribute ("Mean", DoubleValue (m_burstLength->GetValue()));
		par_var->SetAttribute ("Shape", DoubleValue (m_shape));

		m_ParetoDeparture = Simulator::Schedule(t_poisson_arrival + Seconds (par_var->GetValue()),&PPBPApplication::ParetoDeparture, this);

		m_ppbp = Simulator::Schedule(t_poisson_arrival,&PPBPApplication::PPBP, this);
	}

	void PPBPApplication::PoissonArrival()
	{
		NS_LOG_FUNCTION_NOARGS ();
		++m_activebursts;
		if (m_offPeriod) ScheduleNextTx();
	}

	void
	PPBPApplication::ParetoDeparture()
	{
		NS_LOG_FUNCTION_NOARGS ();
		--m_activebursts;
	}

	void
	PPBPApplication::StopApplication() // Called at time specified by Stop
	{
		NS_LOG_FUNCTION_NOARGS ();

		CancelEvents ();
		if(m_socket != 0) m_socket->Close ();
		else NS_LOG_WARN("PPBPApplication found null socket to close in StopApplication");
	}

	void
	PPBPApplication::CancelEvents ()
	{
		NS_LOG_FUNCTION_NOARGS ();
		Simulator::Cancel(m_sendEvent);
		Simulator::Cancel(m_startStopEvent);

		Simulator::Cancel(m_ppbp);
		Simulator::Cancel(m_PoissonArrival);
		Simulator::Cancel(m_ParetoDeparture);
	}

	// Event handlers
	void
	PPBPApplication::StartSending()
	{
		NS_LOG_FUNCTION_NOARGS ();
		m_lastStartTime = Simulator::Now();
		ScheduleNextTx();					// Schedule the send packet event
		ScheduleStopEvent();
	}

	void
	PPBPApplication::StopSending()
	{
		NS_LOG_FUNCTION_NOARGS ();
		CancelEvents();

		ScheduleStartEvent();
	}

	void
	PPBPApplication::ScheduleNextTx()
	{
		NS_LOG_FUNCTION_NOARGS ();
		uint32_t bits = (m_pktSize + 30) * 8;
		Time nextTime(Seconds (bits /
							   static_cast<double>(m_cbrRate.GetBitRate())));

		if (m_activebursts != 0)
		{
			m_offPeriod = false;
			double data_rate = (double) nextTime.GetSeconds() / m_activebursts;
			m_sendEvent = Simulator::Schedule(Seconds(data_rate),&PPBPApplication::SendPacket, this);
		}
		else
		{
			m_offPeriod = true;
		}

	}

	void
	PPBPApplication::ScheduleStartEvent()
	{
		NS_LOG_FUNCTION_NOARGS ();
		m_ppbp = Simulator::Schedule(Seconds(0.0), &PPBPApplication::PPBP, this);
		m_startStopEvent = Simulator::Schedule(Seconds(0.0), &PPBPApplication::StartSending, this);
	}

	void
	PPBPApplication::ScheduleStopEvent()
	{
		NS_LOG_FUNCTION_NOARGS ();
	}

	void
	PPBPApplication::SendPacket()
	{
		NS_LOG_FUNCTION_NOARGS ();
		Ptr<Packet> packet = Create<Packet> (m_pktSize);
		m_txTrace (packet);
		m_socket->Send (packet);
		m_totalBytes += packet->GetSize();
		m_lastStartTime = Simulator::Now();
		ScheduleNextTx();
	}

	void
	PPBPApplication::ConnectionSucceeded(Ptr<Socket>)
	{
		NS_LOG_FUNCTION_NOARGS ();
		m_connected = true;
		ScheduleStartEvent();
	}

	void
	PPBPApplication::ConnectionFailed(Ptr<Socket>)
	{
		NS_LOG_FUNCTION_NOARGS ();
		cout << "PPBPApplication, Connection Failed" << endl;
	}
} // Namespace ns3
