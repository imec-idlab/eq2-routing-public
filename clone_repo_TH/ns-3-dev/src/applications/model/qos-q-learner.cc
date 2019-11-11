#include "ns3/qos-q-learner.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QoSQLearnerApplication");


TypeId
QoSQLearner::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QoSQLearner")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<QoSQLearner> ()
    .AddAttribute ("ReportPacketMetricsToSrc",
                   "overlay network reporting end-to-end delay",
                   BooleanValue(false),
                   MakeBooleanAccessor (&QoSQLearner::m_report_dst_to_src),
                   MakeBooleanChecker ())
   .AddAttribute ("Gamma",
                 "Determines used discount factor.",
                 DoubleValue(0),
                 MakeDoubleAccessor (&QoSQLearner::m_gamma),
                 MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("Verbose",
                   "Determine the amount of output to show, with regard to the learning process.",
                   BooleanValue(false),
                   MakeBooleanAccessor (&QoSQLearner::m_verbose),
                   MakeBooleanChecker ())
   .AddAttribute ("LearnMoreThresh",
                   "Specify when the src should be learning more, because the relevant qtable entry changed too much",
                   DoubleValue(LEARNING_PHASE_START_THRESHOLD),
                   MakeDoubleAccessor (&QoSQLearner::m_learning_threshold),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("EpsilonThreshold",
                   "Determine how often we will choose randomly a path, to keep values up to date.",
                   DoubleValue(DEFAULT_EPSILON_VALUE),
                   MakeDoubleAccessor (&QoSQLearner::m_eps_thresh),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("LearningRate",
                   "Specify learning rate of the QLearner",
                   DoubleValue(DEFAULT_LRN_RATE_VALUE),
                   MakeDoubleAccessor (&QoSQLearner::m_learningrate),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("OutputDataToFile",
                   "Specify wether or not we should be outputting information to output files",
                   BooleanValue(false),
                   MakeBooleanAccessor (&QoSQLearner::m_output_data_to_file),
                   MakeBooleanChecker ())
    .AddAttribute ("PrintQTables",
                   "Print QTables to file or not.",
                   BooleanValue(false),
                   MakeBooleanAccessor(&QoSQLearner::m_print_qtables),
                   MakeBooleanChecker())
   .AddAttribute ("QConvergenceThreshold",
                   "Specify % difference allowable in QValue when looking at converged / not converged",
                   DoubleValue(QTABLE_CONVERGENCE_THRESHOLD),
                   MakeDoubleAccessor (&QoSQLearner::m_qconvergence_threshold),
                   MakeDoubleChecker<double> (0.0, 1.0))
   .AddAttribute ("Rho",
                   "Specify how likely an intermediate node is to optimally forward learning traffic",
                   DoubleValue(Q_INTERMEDIATE_OPTIMAL_RHO),
                   MakeDoubleAccessor (&QoSQLearner::m_rho),
                   MakeDoubleChecker<double> (0.0, 1.0))
   .AddAttribute ("MaxRetries",
                   "Specify how many times we can see a packet before making it into learning traffic",
                   IntegerValue(MAX_RETRY_DEFAULT),
                   MakeIntegerAccessor (&QoSQLearner::m_max_retry),
                   MakeIntegerChecker<int> (0, 65))
    .AddAttribute ("Testing",
                   "Specify test or not",
                   BooleanValue(false),
                   MakeBooleanAccessor (&QoSQLearner::m_in_test),
                   MakeBooleanChecker ())
    .AddAttribute ("Ideal",
                    "Specify ideal or not",
                    BooleanValue(false),
                    MakeBooleanAccessor (&QoSQLearner::m_ideal),
                    MakeBooleanChecker ())
    .AddAttribute ("UseLearningPhases",
                   "Specify wether or not we should be using the learning phases",
                   BooleanValue(false),
                   MakeBooleanAccessor (&QoSQLearner::m_use_learning_phases),
                   MakeBooleanChecker ())
    // .AddAttribute ("SourceAddress",
    //                 "Source address for traffic",
    //                 Ipv4AddressValue("7.7.7.7"),
    //                 MakeIpv4AddressAccessor (&QoSQLearner::m_traffic_source),
    //                 MakeIpv4AddressChecker ())
    // .AddAttribute ("DestinationAddress",
    //                 "Destination address for traffic",
    //                 Ipv4AddressValue("8.8.8.8"),
    //                 MakeIpv4AddressAccessor (&QoSQLearner::m_traffic_destination),
    //                 MakeIpv4AddressChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                   MakeTraceSourceAccessor (&QoSQLearner::m_txTrace),
                   "ns3::Packet::TracedCallback")
                   ;
  return tid;
}


QoSQLearner::QoSQLearner(float e = DEFAULT_EPSILON_VALUE, float lrn_rate = DEFAULT_LRN_RATE_VALUE, float gamma = 0) : QLearner(e, lrn_rate, gamma) {
  m_q_value_when_all_blacklisted = 0;
}

bool QoSQLearner::CheckQLrnHeader_withoutUDP(Ptr<const Packet> p, QoSQLrnHeader& qlrnHeader) {
  p->PeekHeader(qlrnHeader);
  NS_ASSERT(qlrnHeader.IsValid());
  return qlrnHeader.IsValid();
}

bool QoSQLearner::CheckQLrnHeader(Ptr<const Packet> p, QoSQLrnHeader& qlrnHeader) {
  Ptr<Packet> p_copy = p->Copy();
  // add 8 for udp header size that is still attached
  if (p->GetSize() != qlrnHeader.GetSerializedSize()+8) {
    p_copy->PeekHeader(qlrnHeader);
    return false;
  } else { //size is fine
    // Alternatively, solve this with some if loops that put the header back after but this is fine too
    UdpHeader u;
    p_copy->RemoveHeader(u);
    return CheckQLrnHeader_withoutUDP(p_copy, qlrnHeader);
  }
}

void
QoSQLearner::Send(Ipv4Address node_to_notify, uint64_t packet_Uid, Time travel_time, Ipv4Address packet_dst, Ipv4Address packet_src, TrafficType t, bool sender_converged) {
  NS_ASSERT_MSG(node_to_notify != Ipv4Address(UNINITIALIZED_IP_ADDRESS_QINFO_TAG_VALUE), "supposed to be lrn traffic but the qlrn info tag is not initialised."
    << node_to_notify << "time in sim " << Simulator::Now().GetSeconds() << "\n");
  NS_ASSERT_MSG(!m_jittery_qlearner && !m_slow_qlearner, "Dont use this with qos qlearner");

  //TODO ensure next hop is always higher than what is alreadly known about the nieghbour
  // ensure that qte_next_hop is the same for both times it goes into the Send function
  QDecisionEntry* qte_next_hop = GetQTable(t)->GetNextEstim(packet_dst);
  if (GetNode()->GetId() == 0) {
    // NS_LOG_UNCOND("before " << qte_next_hop.GetNextHop());
  }
  if (GetNode()->GetId() == 0) {
    RouteDiffBasedOnType(qte_next_hop,packet_dst,packet_src, t);
  }
  if (GetNode()->GetId() == 0) {
    // NS_LOG_UNCOND("after " << qte_next_hop.GetNextHop());
  }


  // TODO careful here, relies on QTableEntry default constructor at dst to havae a GetREalDelay of 0
  // if (GetNode()->GetId() == 2 && qte_next_hop.GetNextHop() != Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE) )
  // {
  //   std::cout << "hi" << std::endl;
  //   std::cout << qte_next_hop.GetNextHop() << " and loss value " << (
  //   (packet_dst != m_this_node_ip && qte_next_hop.GetNextHop() != Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE) ) ?
  //       GetQTable(t)->GetEntryByRef(packet_dst,qte_next_hop.GetNextHop() /*dst,via*/).GetRealLoss() :
  //       0
  // ) << std::endl; }
  // NS_ASSERT( GetQTable(t)->GetEntryByRef(packet_dst,qte_next_hop.GetNextHop() /*dst,via*/).GetRealLoss() != 0);
  uint64_t next_estim = qte_next_hop->GetQValue().GetInteger();
  // then maybe also try this ?
  // if (AllNeighboursBlacklisted(packet_dst,t) && GetNode()->GetId() == 0) { next_estim = Seconds(101).GetInteger(); }
  if (AllNeighboursBlacklisted(packet_dst,t) /*&& GetNode()->GetId() == 0*/) { next_estim = m_q_value_when_all_blacklisted; }

  Ptr<Packet> packet = Create<Packet> ();

  // std::cout << "going to be doing the find by ref thing here now;.." << std::endl;
  QoSQLrnHeader qLrnHeader (  packet_Uid, travel_time.GetInteger(), next_estim,
                              qte_next_hop->GetRealDelay(), sender_converged, packet_dst,
                              (
                                  (packet_dst != m_this_node_ip
                                    && qte_next_hop->GetNextHop() != Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE) ) ?
                                    GetQTable(t)->GetEntryByRef(packet_dst,qte_next_hop->GetNextHop() /*dst,via*/)->GetRealLoss() :
                                    0
                              ),
                              GetNumRecvPktFromPrevHopToDst(node_to_notify, packet_dst), t);
  // std::cout << "and done....." << std::endl;


  // if (GetNode()->GetId() == 2) {
  //   auto v = GetNumRecvPktFromPrevHopToDst(node_to_notify, packet_dst);
  //   NS_LOG_UNCOND("Im node2, notifying "<<node_to_notify << " abt pkts I received from him = " << v << " and my traffic rec is " << aodvProto->GetRecvTrafficStatistics()
  //   << "\nand the value i put for loss in the packet is " << qLrnHeader.GetRealLoss() << std::endl
  //   << "\nThe current time is " << Simulator::Now() << "    info was contained in pkt " << packet->GetUid() << "\n");
  // }
  packet->AddHeader (qLrnHeader);
  PacketLossTrackingSentTimeQInfo loss_time_sent((Simulator::Now()).GetInteger());
  packet->AddPacketTag(loss_time_sent);
  m_txTrace(packet);

  m_qlrn_socket->SendTo (packet, 0, InetSocketAddress (node_to_notify, QLRN_PORT));
}

void QoSQLearner::UpdateAvgDelay(PacketTimeSentTag ptst_tag,PortNrTag pnt) {
  QLearner::UpdateAvgDelay(ptst_tag,pnt);
  if (!pnt.GetLearningPkt()) {
    // trying this with ns(0), having the real delay (= as before) between nodes would mean the hop right before the dst also stops

    // scrap this entirely, dst always sends 0, others store it in their QTableEntries
    // m_delay_ms_per_dst[m_this_node_ip ] = 0; // (Simulator::Now() - ptst_tag.GetSentTime()).GetInteger();
  }
}

void QoSQLearner::Receive(Ptr<Socket> socket) {
  Address sourceAddress;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
  Ipv4Address sourceIPAddress = InetSocketAddress::ConvertFrom (sourceAddress).GetIpv4 ();

  TrafficType t = OTHER;

  QoSQLrnHeader qlrnHeader;
  packet->RemoveHeader(qlrnHeader);
  if (!qlrnHeader.IsValid ()) {
      NS_FATAL_ERROR("Incorrect QLrnHeader found."); //stop simulation
  }
  t = qlrnHeader.GetTrafficType();

  // std::stringstream ss; qlrnHeader.Print(ss<<std::endl);
  // packet->Print(ss);
  // NS_LOG_UNCOND(ss.str());

  // HANS - put this in comments because according to the compiler this was not used
  // HANS - this one is not used because the line 322 is commented out
  //QDecisionEntry* old_value = GetQTable(t)->GetNextEstim(qlrnHeader.GetPDst());

  uint64_t jitter_val = 0;
  // this one just calculates the jitter value : look at which value (previous delay or this) is biggest, return the biggest - smallest
  if (GetQTable(qlrnHeader.GetTrafficType())->GetNextEstim(qlrnHeader.GetPDst(), sourceIPAddress)->GetRealDelay() != 0 && (qlrnHeader.GetRealDelay()+ qlrnHeader.GetTime()) != 0) {
    jitter_val = ( GetQTable(qlrnHeader.GetTrafficType())->GetNextEstim(qlrnHeader.GetPDst(), sourceIPAddress)->GetRealDelay() < (qlrnHeader.GetRealDelay()+ qlrnHeader.GetTime()) ?
                      (qlrnHeader.GetRealDelay()+ qlrnHeader.GetTime()) - GetQTable(qlrnHeader.GetTrafficType())->GetNextEstim(qlrnHeader.GetPDst(), sourceIPAddress)->GetRealDelay() :
                      GetQTable(qlrnHeader.GetTrafficType())->GetNextEstim(qlrnHeader.GetPDst(), sourceIPAddress)->GetRealDelay() - (qlrnHeader.GetRealDelay()+ qlrnHeader.GetTime()) );
  }

  // Have found the jitter value for this particular packet
  // have also found the currently most recently observed delay value
  //So we effectively set the real delay equal to the previous node's real delay + the time it took for the QInfoTagged packet (not the qlrn header) to travel

  GetQTable(TRAFFIC_A)->GetEntryByRef(qlrnHeader.GetPDst(), sourceIPAddress)->SetRealDelay( qlrnHeader.GetRealDelay() + qlrnHeader.GetTime() );
  GetQTable(TRAFFIC_B)->GetEntryByRef(qlrnHeader.GetPDst(), sourceIPAddress)->SetRealDelay( qlrnHeader.GetRealDelay() + qlrnHeader.GetTime() );
  GetQTable(TRAFFIC_C)->GetEntryByRef(qlrnHeader.GetPDst(), sourceIPAddress)->SetRealDelay( qlrnHeader.GetRealDelay() + qlrnHeader.GetTime() );

  PacketLossTrackingSentTimeQInfo loss_time_sent;
  NS_ASSERT_MSG(packet->PeekPacketTag(loss_time_sent), "We expect this now, so must have this tag present.");
  uint64_t time_value_at_src_of_QLrnHeader = loss_time_sent.GetSentTimeAsInt();

  if (GetNumPktsSentViaNeighbToDst(time_value_at_src_of_QLrnHeader,sourceIPAddress, qlrnHeader.GetPDst()) != 0) {
    float new_loss_value = 1.0;

    if (GetNode()->GetId() == 0) {
      // std::cout << sourceIPAddress << " should have received " << qlrnHeader.GetNumPktsThatSenderReceivedFromMe() << " traffic from me\n"
      //           << " while im thinking i sent " << GetNumPktsSentViaNeighbToDst(time_value_at_src_of_QLrnHeader, sourceIPAddress, qlrnHeader.GetPDst() ) << " at time " << Simulator::Now()   << std::endl
      //           << "m_traffic_pkts_sent = " << GetAODV()->GetTrafficStatistics() << std::endl
      //           << "m_traffic_pkts_recv = " << GetAODV()->GetRecvTrafficStatistics() << std::endl << std::endl;
      // std::cout << "I sent " << GetNumPktsSentViaNeighbToDst(time_value_at_src_of_QLrnHeader,Ipv4Address("10.1.1.2"), qlrnHeader.GetPDst() ) << " via the other.."<< std::endl;
      // std::cout <<float(GetNumPktsSentViaNeighbToDst(time_value_at_src_of_QLrnHeader,sourceIPAddress, qlrnHeader.GetPDst() ))  << " " << GetNumRecvPktFromPrevHopToDst(sourceIPAddress, qlrnHeader.GetPDst() ) << std::endl;
      // std::cout<< " q info was sent at " << loss_time_sent.GetSentTime()<<std::endl;
      // std::cout << "      loss info from pkt : " << qlrnHeader.GetRealLoss() << std::endl;
    }


    int difference = GetNumPktsSentViaNeighbToDst(time_value_at_src_of_QLrnHeader,sourceIPAddress, qlrnHeader.GetPDst()) - qlrnHeader.GetNumPktsThatSenderReceivedFromMe() ;
    if (difference < 0) {
      NS_FATAL_ERROR("this shouldnt happen, nh received more packets than we sent..? we sent " << GetNumPktsSentViaNeighbToDst(time_value_at_src_of_QLrnHeader,sourceIPAddress, qlrnHeader.GetPDst())
                  << "and they claim " << qlrnHeader.GetNumPktsThatSenderReceivedFromMe());
    } else if (difference == 0 || difference == 1) {
      new_loss_value = qlrnHeader.GetRealLoss() != 0 ? qlrnHeader.GetRealLoss()/10000.0 : 1;
    } else {
      new_loss_value =
            (qlrnHeader.GetRealLoss() != 0 ? qlrnHeader.GetRealLoss()/10000.0 : 1) *
            qlrnHeader.GetNumPktsThatSenderReceivedFromMe() /
            (float(GetNumPktsSentViaNeighbToDst(time_value_at_src_of_QLrnHeader,sourceIPAddress, qlrnHeader.GetPDst() ) ) ) ;
      // std::cout << " at node " << m_name << " difference" << difference << "      and then loss is " << new_loss_value << std::endl;
    }
    // Removed assert difference ==1 , old from debugging i think

    SetRealLossForOutput(sourceIPAddress,qlrnHeader.GetPDst(),new_loss_value); //stored as a real float

    GetQTable(TRAFFIC_A)->GetEntryByRef(qlrnHeader.GetPDst(), sourceIPAddress)->SetRealLoss( new_loss_value * 10000); //stored as uint16 so *10000 to get ab,cd as value
    GetQTable(TRAFFIC_B)->GetEntryByRef(qlrnHeader.GetPDst(), sourceIPAddress)->SetRealLoss( new_loss_value * 10000);
    GetQTable(TRAFFIC_C)->GetEntryByRef(qlrnHeader.GetPDst(), sourceIPAddress)->SetRealLoss( new_loss_value * 10000);
  }

  NS_LOG_DEBUG( m_name << "learning info about " << qlrnHeader.GetPDst() <<" from packet ID " << qlrnHeader.GetPktId()
            << " : travel time was " << Time::FromInteger(qlrnHeader.GetTime(), Time::NS).As(Time::MS) << " and next estim : " << Time::FromInteger(qlrnHeader.GetNextEstim(), Time::NS)
            << ". This info was contained in pkt " << packet->GetUid() << " sent by " <<  InetSocketAddress::ConvertFrom (sourceAddress).GetIpv4 ());
  uint64_t unpunished_new_q_value = GetQTable(t)->CalculateNewQValue(sourceIPAddress, //get the neighbour that we chose as next hop
                                      qlrnHeader.GetPDst(),   //the actual destination of the packet ( to know which entry in the QTable to update)
                                      m_packet_info.GetPacketQueueTime(qlrnHeader.GetPktId()), //the time the packet spent in the queue
                                      Time::FromInteger(qlrnHeader.GetTime(), Time::NS) /*- m_packet_info.GetPacketQueueTime(qlrnHeader.GetPktId()) old code (also in qlearner.cc)*/, // //the time the packet spent in the air (so time between sent and arrival)
                                      Time::FromInteger(qlrnHeader.GetNextEstim(), Time::NS) //and also the next hop's estimate, as we need that
                                    );

  if (qlrnHeader.GetNextEstim() >= uint64_t(Seconds(30).GetInteger()) &&
      m_q_value_when_all_blacklisted == 0 &&
      GetQTable(t)->GetEntryByRef(qlrnHeader.GetPDst(), sourceIPAddress)->IsBlackListed() ) {
    m_q_value_when_all_blacklisted = qlrnHeader.GetNextEstim()-Seconds(10).GetInteger();
  }

  if (GetNode()->GetId() == 0 ) {
    // NS_LOG_UNCOND("at node " << m_name << " abt pkt " <<  qlrnHeader.GetPktId());
    // NS_LOG_UNCOND("get real delay = " << qlrnHeader.GetRealDelay() << "    annd travel time of the last qlrn info tag?" << qlrnHeader.GetTime());
    // NS_LOG_UNCOND("Metric DELAY:" << GetQTable(qlrnHeader.GetTrafficType()).GetNextEstim(qlrnHeader.GetPDst(), sourceIPAddress).GetRealDelay() );
    // NS_LOG_UNCOND("Metric JITTER:" << jitter_val );
    // NS_LOG_UNCOND("Metric LOSS:" << GetQTable(qlrnHeader.GetTrafficType()).GetEntryByRef(qlrnHeader.GetPDst(), sourceIPAddress).GetRealLoss());
  }

  ApplyMetricsToQValue(qlrnHeader.GetPDst(), sourceIPAddress, unpunished_new_q_value, t,
              GetQTable(qlrnHeader.GetTrafficType())->GetNextEstim(qlrnHeader.GetPDst(), sourceIPAddress)->GetRealDelay(),
               jitter_val, GetQTable(qlrnHeader.GetTrafficType())->GetEntryByRef(qlrnHeader.GetPDst(), sourceIPAddress)->GetRealLoss() );

  if (qlrnHeader.GetPDst() != sourceIPAddress) {
    NS_LOG_DEBUG( m_name << "[N] ALSO LEARNING abt " << sourceIPAddress <<" from packet ID " << qlrnHeader.GetPktId()
              << " : travel time was " << Time::FromInteger(qlrnHeader.GetTime(), Time::NS).As(Time::MS)
              << " GetPacketQueueTime time was " << m_packet_info.GetPacketQueueTime(qlrnHeader.GetPktId())
              << ". Contained in pkt " << packet->GetUid() << " sent by " <<  sourceIPAddress);

  GetQTable(t)->Update(sourceIPAddress, //get the neighbour that we chose as next hop
                    sourceIPAddress,
    /*experimental*/m_packet_info.GetPacketQueueTime(qlrnHeader.GetPktId()), //the time the packet spent in the queue for nexthop neighbours, this should be 0
                    Time::FromInteger(qlrnHeader.GetTime(), Time::NS) /*- m_packet_info.GetPacketQueueTime(qlrnHeader.GetPktId()) old code (also in qlearner.cc)*/, // //the time the packet spent in the air (so time between sent and arrival)
                    Time::FromInteger(0, Time::NS) //and also the next hop's estimate, as we need that
                  );
  }
  // HANS - put this in comments because according to the compiler this was not used
  //QDecisionEntry* new_value = GetQTable(t)->GetNextEstim(qlrnHeader.GetPDst(),old_value->GetNextHop());


  GetQTable(TRAFFIC_A)->GetEntryByRef(qlrnHeader.GetPDst(), InetSocketAddress::ConvertFrom (sourceAddress).GetIpv4 ())->SetSenderConverged(qlrnHeader.GetSenderConverged());
  GetQTable(TRAFFIC_B)->GetEntryByRef(qlrnHeader.GetPDst(), InetSocketAddress::ConvertFrom (sourceAddress).GetIpv4 ())->SetSenderConverged(qlrnHeader.GetSenderConverged());
  GetQTable(TRAFFIC_C)->GetEntryByRef(qlrnHeader.GetPDst(), InetSocketAddress::ConvertFrom (sourceAddress).GetIpv4 ())->SetSenderConverged(qlrnHeader.GetSenderConverged());

  Ipv4Address destination = qlrnHeader.GetPDst() ;
  if (qlrnHeader.GetSenderConverged()) {
    //can do this instead of checking them one by one, exhaustively
    NS_ASSERT_MSG(std::find(m_traffic_destinations.begin(), m_traffic_destinations.end(), destination) != m_traffic_destinations.end()
      ,"it shouldnt have been true but it is/was ... \n" << qlrnHeader << "\n"<<destination);
  }
  if (GetQTable(t)->HasConverged(destination) && m_num_applications >= 1 && m_use_learning_phases && m_learning_phase[destination] && m_my_sent_traffic_destination == destination) {
    NS_LOG_UNCOND(m_name << "(qos) Im in learning phase, all choices have converged, time ( " << Simulator::Now().As(Time::S) << " ) to stop learning & start traffic. pkt id:" << packet->GetUid());
    StopLearningStartTraffic(destination);
    NS_ASSERT(GetNode()->GetId() == 0); // old assert for when node0 only has 1 next_hop
    // NS_FATAL_ERROR("i dont want this to happen?"); //-- but it happens anyway ?? the fk --- only happens in 0, because it only has 1 choice
  } else if (GetQTable(t)->HasConverged(destination, true ) &&  m_num_applications >= 1  && m_use_learning_phases && m_learning_phase[destination] && m_my_sent_traffic_destination == destination) {
    NS_LOG_UNCOND(m_name << "(qos) Im in learning phase, best choice has converged, time ( " << Simulator::Now().As(Time::S)
    << " ) to stop learning & start traffic. pkt id:" << packet->GetUid() << "\n" << GetQTable(t)->PrettyPrint() << "\n");
    // NS_FATAL_ERROR("stop");
    StopLearningStartTraffic(destination);
  } else if (GetQTable(t)->HasConverged(destination, true ) && m_use_learning_phases && m_learning_phase[destination]) {
    SetLearningPhase(false, destination); //-- so the issue lies here, "best estim" is not necessarily really the best estim, and that ruins the simulation for 15 straight line nodes!
  } else if (GetQTable(t)->HasConverged(destination) && m_use_learning_phases && m_learning_phase[destination]) {
    SetLearningPhase(false, destination); //Decide on this part still... best (i think) is to have only one  converge since thats all we need
  }


  // Because for one entyr it might be told to learn more, then another will send QInfo and the
  // decision may be to learn less. As these have different variables, they might cancel eachother out!
  // bool to_dst_learn_less = GetQTable(t)->GetEntryByRef(qlrnHeader.GetPDst(),sourceIPAddress).LearnLess();
  // bool to_dst_learn_more = GetQTable(t)->GetEntryByRef(qlrnHeader.GetPDst(),sourceIPAddress).LearnMore();
  bool to_dst_learn_less = GetQTable(t)->LearnLess(qlrnHeader.GetPDst() );
  bool to_dst_learn_more = GetQTable(t)->LearnMore(qlrnHeader.GetPDst() );
  NS_ASSERT_MSG(!(to_dst_learn_more && to_dst_learn_less) , "Cant have both at the same time, that would be conflicing ideas.");

  /*
   * dont increase or decrease learning for the neighbour, we care only about the dst!
   * values for neighb are much smaller so are %change more
   */
  bool to_neigh_learn_less = false; // GetQTable(t)->GetEntryByRef(sourceIPAddress,sourceIPAddress).LearnMore();
  bool to_neigh_learn_more = false; // GetQTable(t)->GetEntryByRef(sourceIPAddress,sourceIPAddress).LearnMore();

  if ( (to_dst_learn_more || to_neigh_learn_more ) && m_small_learning_stream && m_my_sent_traffic_destination == destination /* dont mess with traffic if its not our dst*/ &&
        m_use_learning_phases && !m_learning_phase[destination] && m_num_applications == 2 && learning_traffic_applications/*->GetN()==1*/ != 0 && real_traffic/*->GetN()==1*/ != 0) {
    // std::cout << (to_dst_learn_more ? "to dst learn more":"not to dst learn more" ) << std::endl;
    // std::cout << (to_neigh_learn_more ? "to neigh learn more":"not to neigh learn more" ) << std::endl;

    try {
      dynamic_cast<OnOffApplication&> ( *(PeekPointer(learning_traffic_applications)) ).IncreaseAmountOfTraffic(0,1.5); //--VANHIER
    } catch (std::bad_cast & e) {
      NS_FATAL_ERROR("bad cast was thrown & caught. learning_traffic_applications should be of the OnOffApplication variety though." << e.what() );
    }

  } else if ( ( to_dst_learn_less || to_neigh_learn_less ) && m_small_learning_stream && m_my_sent_traffic_destination == destination /* dont mess with traffic if its not our dst*/ &&
                m_use_learning_phases && !m_learning_phase[destination] && m_num_applications == 2 && learning_traffic_applications/*->GetN()==1*/ != 0 && real_traffic/*->GetN()==1*/ != 0) {
    // std::cout << (to_dst_learn_less ? "to dst learn less":"not to dst learn less" ) << std::endl;
    // std::cout << (to_neigh_learn_less ? "to neigh learn less":"not to neigh learn less" ) << std::endl;

    try {
      dynamic_cast<OnOffApplication&> ( *(PeekPointer(learning_traffic_applications/*->Get(0)*/)) ).ReduceAmountOfTraffic(0,0.5); //--VANHIER
    } catch (std::bad_cast & e) {
      NS_FATAL_ERROR("bad cast was thrown & caught. learning_traffic_applications should be of the OnOffApplication variety though." << e.what() );
    }

  }
  return;
}



void QoSQLearner::ApplyMetricsToQValue(Ipv4Address dst, Ipv4Address next_hop, uint64_t unpunished_value, TrafficType t,
    uint64_t delay_metric, uint64_t jitter_metric, float packet_loss_metric) {
  std::vector<TrafficType> TrafficTypes = {TRAFFIC_A, TRAFFIC_B, TRAFFIC_C};

  NS_ASSERT_MSG(packet_loss_metric <= 10000, "Having more packets received than we sent? i think not!");

  for (auto i : TrafficTypes ) {

    QDecisionEntry* entry =  GetQTable(i)->GetEntryByRef(dst, next_hop) ;
    uint64_t old_value = entry->GetQValue().GetInteger();
    if (t == i) {
      if (unpunished_value > old_value) {
        // if the unpunished value, any punishments dissapear as we will be punishing it more (most likely)
        // and otherwise, its still higher than the punished value
        entry->SetCoefficientTally(1.0);
      }
      old_value = unpunished_value;
    }
    float delay_coefficient = 1.0, jitter_coefficient = 1.0, packet_loss_coefficient = 1.0;

    delay_coefficient = ( delay_metric < TrafficTypeReqsMap[i].GetDelayMax() ? 1.0 : 1+(delay_metric / TrafficTypeReqsMap[i].GetDelayMax()));
    jitter_coefficient = ( jitter_metric < TrafficTypeReqsMap[i].GetJitterMax() ? 1.0 : 2+(jitter_metric / TrafficTypeReqsMap[i].GetJitterMax())); //jitter is lower qvals so mult more

    if (packet_loss_metric == 0.0) { } else {
      float actual_percent_loss = 1- (packet_loss_metric / 10000.0);
      if (actual_percent_loss > 10 * TrafficTypeReqsMap[i].GetRandomLossMax()) {
        packet_loss_coefficient = 12.5;
      } else if (actual_percent_loss > 2 * TrafficTypeReqsMap[i].GetRandomLossMax()) {
        packet_loss_coefficient = 8;
      } else if (actual_percent_loss > TrafficTypeReqsMap[i].GetRandomLossMax()) {
        packet_loss_coefficient = 4;
      } else {
        packet_loss_coefficient = 1.0;
        // if (GetNode()->GetId() == 0) { std::cout << dst << " " << next_hop << " " << actual_percent_loss << " " << TrafficTypeReqsMap[i].GetRandomLossMax() <<
        // GetQTable(t)->PrettyPrint("yeah this is the table") << std::endl;;}
      }
    }

    if (GetNode()->GetId() == 0 && i == TRAFFIC_A
          && (delay_coefficient *jitter_coefficient * packet_loss_coefficient )!= 1.0) {
      // NS_LOG_UNCOND(m_name << " receives info about " << dst << " via " << next_hop ) ;
      // NS_LOG_UNCOND("Metric DELAY:" << delay_metric << "  and coeff is " << delay_coefficient);
      // NS_LOG_UNCOND("Metric JITTER:" << jitter_metric << "  and coeff is " << jitter_coefficient);
      // NS_LOG_UNCOND("Metric LOSS:" << packet_loss_metric << " and coeff is " << packet_loss_coefficient);
      // NS_FATAL_ERROR("stop");
    }

    // seen_packet_before_coefficient = TODO do this ? (might even be a point to having this in the q-learner itself )

    if (entry->GetCoefficientTally() > 1.0) {

      entry->AddStrike(); // start here instead of after the 1st time its bad metrics, why ? idk!
      // entry->SetCoefficientTally(delay_coefficient * jitter_coefficient * packet_loss_coefficient);

      float curr_tally = entry->GetCoefficientTally();
      float new_tally = delay_coefficient * jitter_coefficient * packet_loss_coefficient;
      float coefficient = 1.0;
      // if (new_tally == 1.0 && curr_tally != 1.0 && GetNode()->GetId() == 0 && next_hop == Ipv4Address("10.1.1.3")) {
        // NS_LOG_UNCOND("Metric DELAY:" << delay_metric << "  and coeff is " << delay_coefficient);
        // NS_LOG_UNCOND("Metric JITTER:" << jitter_metric << "  and coeff is " << jitter_coefficient);
        // NS_LOG_UNCOND("Metric LOSS:" << packet_loss_metric << " and coeff is " << packet_loss_coefficient);
        // std::cout << "issues have gone for next hop " << next_hop << " to dst " << dst << std::endl;
        // -- this does happen because 10 1 1 3 occasionally switches to using 10 1 1 1 as a next hop
        // NS_ASSERT(false);
      // }

      if (curr_tally >= new_tally ) { // experimental, so it continually keeps punishing
        // if (GetNode()->GetId() == 0 && dst == Ipv4Address("10.1.1.8") && next_hop == Ipv4Address ("10.1.1.3") ) {
        //   std::cout << entry->GetCoefficientTally() << std::endl;
        // }

        uint64_t new_value = (old_value * (curr_tally + new_tally) / 2 < Seconds(100).GetInteger() ? old_value * (curr_tally + new_tally) / 2 :Seconds(100).GetInteger());
        GetQTable(i)->SetQValueWrapper(dst,next_hop,Time::FromInteger(new_value, Time::NS));
        entry->SetCoefficientTally((curr_tally + new_tally) / 2.0);

        // For some reason, enabling this wrecks it again
        // entry->SetCoefficientTally((1-m_learningrate) * entry->GetCoefficientTally());
        //do nothing
        if (new_tally == 1.0) {
          entry->DeductStrike();
        }
      } else {
        coefficient = new_tally / curr_tally;
        uint64_t new_value = (old_value * coefficient < Seconds(100).GetInteger() ? old_value * coefficient :Seconds(100).GetInteger());
        if (GetNode()->GetId() == 0 && old_value != 0) {
          NS_LOG_DEBUG ( "(keep punish) old value was = " << old_value << "  and new value is " << new_value );
        }
        GetQTable(i)->SetQValueWrapper(dst,next_hop,Time::FromInteger(new_value, Time::NS));
        entry->SetCoefficientTally(delay_coefficient * jitter_coefficient * packet_loss_coefficient);
      }
    }
    else {
      // if ((delay_coefficient * jitter_coefficient * packet_loss_coefficient) != 1.0) {
      //   NS_LOG_UNCOND("Metric DELAY:" << delay_metric << "  and coeff is " << delay_coefficient);
      //   NS_LOG_UNCOND("Metric JITTER:" << jitter_metric << "  and coeff is " << jitter_coefficient);
      //   NS_LOG_UNCOND("Metric LOSS:" << packet_loss_metric << " and coeff is " << packet_loss_coefficient);
      // }
      float coefficient = ( delay_coefficient * jitter_coefficient * packet_loss_coefficient);
      uint64_t new_value = (old_value * coefficient < Seconds(100).GetInteger() ? old_value * coefficient :Seconds(100).GetInteger());

      if (( delay_coefficient * jitter_coefficient * packet_loss_coefficient) > 1.0 && GetNode()->GetId() == 0) {
        NS_LOG_DEBUG ( "(norm) old value was = " << Time::FromInteger(old_value, Time::NS).As(Time::MS) << "  and new value is " << Time::FromInteger(new_value, Time::NS).As(Time::MS) );
      }

      /* careful here, dont want to inadvertendly cause it to converge if all coeffs are equal to 1! */
      // --- this should be resolved, this is now the onyl call to set QValue in the QoSQ case for dst
      GetQTable(i)->SetQValueWrapper(dst,next_hop,Time::FromInteger(new_value, Time::NS));
      entry->SetCoefficientTally(delay_coefficient * jitter_coefficient * packet_loss_coefficient);
    }
    // if (packet_loss_coefficient * delay_coefficient * jitter_coefficient != 1 && GetNode()->GetId() == 0) {
    //   NS_LOG_UNCOND( traffic_type_to_traffic_string(i) << "  " << "old value was = " << old_value << "  and new value is " << GetQTable(i)->GetEntryByRef(dst, next_hop).GetQValue().GetInteger() );
    // }

  }

}

void
QoSQLearner::StartApplication (void) {
  m_q_value_when_all_blacklisted = (GetNode()->GetId() == 0 ? Seconds(100).GetInteger():0);
  m_this_node_ip = GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
  if (m_output_data_to_file && IamAmongTheDestinations() ) {
    std::stringstream ss; ss << "out_stats_qosq_node"<<GetNode()->GetId()<<".csv";
    m_output_filestream = Create<OutputStreamWrapper> (ss.str(), std::ios::out);
    *(m_output_filestream->GetStream ()) << "pktID,currTime,delay,initial_estim,learning,metrics,second_to_last_hop,trafficType\n";
  }

  QLearner::StartApplication();
  if (m_qlrn_socket) {
    m_qlrn_socket->SetRecvCallback( MakeCallback (&QoSQLearner::Receive, this));
  }

  if (GetNode()->GetId() == 0) {
    NS_ASSERT_MSG(m_num_applications == 2 , "Two applications only if we're doing QoS QLearning please . . . instead value of num appl was " << m_num_applications <<"  "<< m_name<< std::endl);
  }

}

} //namespace ns3
