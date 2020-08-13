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
#include "q-learner.h"

/*
  (RFC 3561)
   For each valid route maintained by a node as a routing table entry,
   the node also maintains a list of precursors that may be forwarding
   packets on this route.  These precursors will receive notifications
   from the node in the event of detection of the loss of the next hop
   link.  The list of precursors in a routing table entry contains those
   neighboring nodes to which a route reply was generated or forwarded.

  so precursors are nodes that may use the node for forwarding of packets
  and that will be notified if a next_hop goes down, good to know
*/

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QLearnerApplication");

NS_OBJECT_ENSURE_REGISTERED (QLearner);

int traffic_string_to_port_number (std::string traffic) {
  if (!traffic.compare("trafficA") ) {
    return PORT_NUMBER_TRAFFIC_A;
  } else if (!traffic.compare("trafficB") ) {
    return PORT_NUMBER_TRAFFIC_B;
  } else if (!traffic.compare("trafficC") ) {
    return PORT_NUMBER_TRAFFIC_C;
  } else {
    return 0;
  }
}

TrafficType traffic_string_to_traffic_type(std::string traffic) {
  std::stringstream test(traffic);
  std::string segment;

  while(std::getline(test, segment, '/')) {  }
  traffic = segment;

  TrafficType t = TRAFFIC_C;
  if (!traffic.compare("voip") || !traffic.compare("trafficA")) {
    t = TRAFFIC_A;
  } else if (!traffic.compare("video") || !traffic.compare("trafficB")) {
    t = TRAFFIC_B;
  }
  return t;
}

std::string traffic_type_to_traffic_string(TrafficType t) {
  std::string ret;
  if (t == OTHER) {
    ret = "Unknown traffic type. (or aodv or icmp)";
  } else if (t == WEB) {
    ret = "WEB traffic";
  } else if (t == VIDEO || t == TRAFFIC_B) {
    ret = "VIDEO / TRAFFIC_B traffic";
  } else if (t == VOIP || t == TRAFFIC_A) {
    ret = "VOIP / TRAFFIC_A traffic";
  } else if (t == ICMP){
    ret = "ICMP traffic";
  } else if (t == UDP_ECHO){
    ret = "UDP ECHO traffic";
  } else if (t == TRAFFIC_C) {
    ret = "OTHER / TRAFFIC_C traffic";
  } else {
    std::cout << t << std::endl;
    NS_FATAL_ERROR ("Unknown traffic type.");
  }
  return ret;
}

/// Port for QLRN control traffic
const uint32_t QLearner::QLRN_PORT = 404;

TypeId
QLearner::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QLearner")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<QLearner> ()
    .AddAttribute ("Verbose",
                   "Determine the amount of output to show, with regard to the learning process.",
                   BooleanValue(false),
                   MakeBooleanAccessor (&QLearner::m_verbose),
                   MakeBooleanChecker ())
    .AddAttribute ("EpsilonThreshold",
                   "Determine how often we will choose randomly a path, to keep values up to date.",
                   DoubleValue(DEFAULT_EPSILON_VALUE),
                   MakeDoubleAccessor (&QLearner::m_eps_thresh),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("Gamma",
                  "Determines used discount factor.",
                  DoubleValue(0),
                  MakeDoubleAccessor (&QLearner::m_gamma),
                  MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("LearningRate",
                   "Specify learning rate of the QLearner",
                   DoubleValue(DEFAULT_LRN_RATE_VALUE),
                   MakeDoubleAccessor (&QLearner::m_learningrate),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("QConvergenceThreshold",
                    "Specify % difference allowable in QValue when looking at converged / not converged",
                    DoubleValue(QTABLE_CONVERGENCE_THRESHOLD),
                    MakeDoubleAccessor (&QLearner::m_qconvergence_threshold),
                    MakeDoubleChecker<double> (0.0, 50.0))
    .AddAttribute ("Rho",
                    "Specify how likely an intermediate node is to optimally forward learning traffic",
                    DoubleValue(Q_INTERMEDIATE_OPTIMAL_RHO),
                    MakeDoubleAccessor (&QLearner::m_rho),
                    MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("LearnMoreThresh",
                    "Specify when the src should be learning more, because the relevant qtable entry changed too much",
                    DoubleValue(LEARNING_PHASE_START_THRESHOLD),
                    MakeDoubleAccessor (&QLearner::m_learning_threshold),
                    MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("MaxRetries",
                    "Specify how many times we can see a packet before making it into learning traffic",
                    IntegerValue(MAX_RETRY_DEFAULT),
                    MakeIntegerAccessor (&QLearner::m_max_retry),
                    MakeIntegerChecker<int> (0, 70))
    .AddAttribute ("Testing",
                   "Specify test or not",
                   BooleanValue(false),
                   MakeBooleanAccessor (&QLearner::m_in_test),
                   MakeBooleanChecker ())
    .AddAttribute ("PrintQTables",
                   "Print QTables to file or not.",
                   BooleanValue(false),
                   MakeBooleanAccessor(&QLearner::m_print_qtables),
                   MakeBooleanChecker())
    .AddAttribute ("Ideal",
                    "Specify ideal or not",
                   BooleanValue(false),
                   MakeBooleanAccessor (&QLearner::m_ideal),
                   MakeBooleanChecker ())
    .AddAttribute ("UseLearningPhases",
                   "Specify wether or not we should be using the learning phases",
                   BooleanValue(false),
                   MakeBooleanAccessor (&QLearner::m_use_learning_phases),
                   MakeBooleanChecker ())
    .AddAttribute ("OutputDataToFile",
                   "Specify wether or not we should be outputting information to output files",
                   BooleanValue(false),
                   MakeBooleanAccessor (&QLearner::m_output_data_to_file),
                   MakeBooleanChecker ())
    // .AddAttribute ("SourceAddress",
    //                "Source address for traffic",
    //                Ipv4AddressValue("7.7.7.7"),
    //                MakeIpv4AddressAccessor (&QLearner::m_traffic_source),
    //                MakeIpv4AddressChecker ())
    // .AddAttribute ("DestinationAddress",
    //                "Destination address for traffic",
    //                Ipv4AddressValue("8.8.8.8"),
    //                MakeIpv4AddressAccessor (&QLearner::m_traffic_destination),
    //                MakeIpv4AddressChecker ())
     .AddAttribute ("ReportPacketMetricsToSrc",
                    "overlay network reporting end-to-end delay",
                    BooleanValue(false),
                    MakeBooleanAccessor (&QLearner::m_report_dst_to_src),
                    MakeBooleanChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                   MakeTraceSourceAccessor (&QLearner::m_txTrace),
                   "ns3::Packet::TracedCallback")
                   ;
  return tid;
}

QLearner::QLearner (float eps_param = DEFAULT_EPSILON_VALUE, float learning_rate = DEFAULT_LRN_RATE_VALUE, float gamma = DEFAULT_GAMMA_VALUE)
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
  m_qlrn_socket = 0;
  m_verbose = false;
  m_packet_info = PacketTable();

  m_eps_thresh = eps_param;
  m_gamma = gamma;
  m_epsilon = CreateObject<UniformRandomVariable> ();
  m_epsilon->SetAttribute ("Min", DoubleValue (0.0));
  m_epsilon->SetAttribute ("Max", DoubleValue (1.0));

  m_learningrate = learning_rate;
  m_name = "NOT INITIALIZED";

  m_in_test = false;

  m_ideal = false;

  m_control_packets_sent = 0;
  m_other_qlearners = std::map<Ipv4Address,Ptr<QLearner> >();

  m_learning_phase = std::map<Ipv4Address,bool>() ; //initially, learning!
  m_learning_threshold = LEARNING_PHASE_START_THRESHOLD;
  m_apply_jitter = false;
  m_jittery_qlearner = false;
  m_slow_qlearner = false;
  m_output_data_to_file = false;
  m_print_qtables = false;

  m_report_dst_to_src = false;

  m_num_applications = 0;

  m_output_filestream = 0;
  m_use_learning_phases = false;
  m_traffic_sources = std::vector<Ipv4Address>();
  m_traffic_destinations = std::vector<Ipv4Address>();
  m_my_sent_traffic_destination = Ipv4Address("99.99.99.99");

  tmp = std::set<unsigned int>();

  m_qtable = QTable();
  m_qtable_voip = QTable();
  m_qtable_video = QTable();

  neighbours = std::vector<Ipv4Address>();
  aodvProto = 0;
  learning_traffic_applications = 0;
  real_traffic = 0;

  m_small_learning_stream = false;

  m_running_avg_latency = std::make_pair<int,float>(0,0);

  m_delay = 0;
  m_qconvergence_threshold = 0.025;
  m_rho = 0.99;
  m_max_retry = 4;
  m_prev_delay = 0;

  m_packet_sent_stats_per_neighb_per_time = std::map<std::pair<Ipv4Address,Ipv4Address> ,std::vector<std::tuple<uint64_t,uint64_t,float> > >();
  m_packet_recv_stats_per_neighb          = std::map<std::pair<Ipv4Address,Ipv4Address> ,std::pair<uint64_t,float> >();

  m_backup_per_prev_hop_for_unusable_delay = std::map<Ipv4Address,Time>();
  m_prev_delay_per_prev_hop = std::map<Ipv4Address,uint64_t>();
}

QLearner::~QLearner()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  aodvProto = 0;
  learning_traffic_applications = 0;
  real_traffic = 0;
  m_qlrn_socket = 0;
  m_epsilon = 0;
  m_other_qlearners = std::map<Ipv4Address, Ptr<QLearner> >();
  m_output_filestream = 0;
}

std::vector<Ipv4Address>
QLearner::FindNeighboursManual(void)
{
  std::list<Ipv4Address> ret = std::list<Ipv4Address>();
  if (GetNode()->GetId() == 0) {
    aodv::RoutingTable rt = aodvProto->GetRoutingTable ();//.LookupRoute (interfaces.GetAddress (numberOfNodes-1), toDst);
    aodvProto->SendHello ();
  }
  // ret.insert(neighb);

  /**
   * EITHER:
   *  get it from the AODV rtable somehow (by making private things public or adding more members)
   * OR
   *  send a UDP message (f.ex. using the AODV hello) but then somehow the traffic has to be differed
   */

  NS_ASSERT_MSG(false, "Not (completely) implemented and not suited for use. UDP Bcast should work though.\n");

  return std::vector<Ipv4Address>();
}

std::vector<Ipv4Address>
QLearner::FindNeighboursAODV(void)
{
   /**
    * Find neighbors using aodv's neighbor table instead of sending our own UDP packet and checking responses
    *
    * It is not entirely clear what the best method is or may be, this will have to be found out later.
    * Currently I believe the method of GetNeighbors()->GetVector() to be superior, though I've only been able to
    * prove this empirically for now.
    */
  std::vector<Ipv4Address> neighbours_fromGetVector;
  std::vector<Ipv4Address> neighbours_fromGetListOfDst;

  if (aodvProto->GetNeighbors()->GetVector().size() > 0) {
    for (auto const& x : aodvProto->GetNeighbors()->GetVector()) {
      neighbours_fromGetVector.push_back(x.m_neighborAddress);
    }
  }

  return neighbours_fromGetVector;

  /*
  This is another method but relies on knowing the adresses beforehand. Something can most likely be done to remedy this, but for now I'm using the vector method impleemented above.
  */
  // aodv::RoutingTableEntry toDst;
  // std::list<Ipv4Address> possible_addresses = std::list<Ipv4Address>{Ipv4Address("10.1.1.1"),Ipv4Address("10.1.1.2"),Ipv4Address("10.1.1.3"),Ipv4Address("10.1.1.4"),Ipv4Address("10.1.1.5")};
  // for (auto const& x : possible_addresses) {
  //   std::map<Ipv4Address,uint32_t> unreachable;
  //   aodvProto->GetRoutingTable().GetListOfDestinationWithNextHop (x, unreachable);
  //   for (auto const& x : unreachable)
  //   {
  //     neighbours_fromGetListOfDst.push_back(x.first);
  //   }
  // }
  // return neighbours_fromGetListOfDst;
}

bool
QLearner::Route(Ptr<Ipv4Route> route, Ptr<Packet> p, const Ipv4Header& header) {
  /**
   *  Each node has a neighbour table that it uses to choose a next hop for a destination
   *  Prob: for each destination, we need to choose one of these neighbours
   *  Thus we have a table of destinations x neighbours that we have to fill with estimates for the time it takes
   *  to forward a packet from the current node to the destination via that neighbor.
   */
  return Route(route, p, header.GetDestination(), header.GetSource());
}

void QLearner::RouteHelper(Ptr<Ipv4Route> route, Ipv4Address dst, TrafficType t, uint64_t& initial_estim, int mode, Ptr<Packet> p /* for output only ...*/) {
  bool unconverged_entries_only, optimum_route_check_up;
  if      (mode == 0) { unconverged_entries_only = false; optimum_route_check_up = false; } // So just any random one
  else if (mode == 1) { unconverged_entries_only = true;  optimum_route_check_up = false; } // only the unconverged entries!
  else if (mode == 2) { unconverged_entries_only = false; optimum_route_check_up = true; }  // explore optimum, whatever it is
  else { NS_FATAL_ERROR("Not supporting any mode values higher than 2 or lower than 0. Value in error: " << mode); }

  Ipv4Address next_hop = route->GetGateway();

  // p->AddPacketTag(RandomDecisionTag(true)); // needed ?

  // 2 options here : either learn the optimal (converged, hopefully) route, or just take any random one , which may again be a converged one
  QTableEntry random_estimate;
  if (optimum_route_check_up) { random_estimate = GetQTable(t).GetNextEstim(dst); }
  else { random_estimate = GetQTable(t).GetRandomEstim(dst, unconverged_entries_only); }

  initial_estim = random_estimate.GetQValue().GetInteger();
  route->SetGateway(random_estimate.GetNextHop()  );

  NS_LOG_DEBUG(m_name << "Exploration " << (unconverged_entries_only? "(non converged entries only)":"(any entries)") <<" replaces " << next_hop << " by  "
      << route->GetGateway() << " on packet id " << p->GetUid() << " destined for " << dst );
}

bool
QLearner::Route(Ptr<Ipv4Route> route, Ptr<Packet> p, const Ipv4Address& dst, const Ipv4Address& src) {
  QLrnInfoTag tag;

  PortNrTag pnt;

  PacketTimeSentTagPrecursorAB ptab;
  PacketTimeSentTagPrecursorCT ptct;
  PacketTimeSentTag ptst_tag;

  uint64_t initial_estim;

  std::stringstream ss;
  p->Print(ss);

  //Get traffic type of packet;
  TrafficType t = OTHER;
  aodvProto->CheckTraffic(p, t);
  QLrnHeader qlrnHeader;
  QoSQLrnHeader qosQlrnHeader;

  if (p->PeekPacketTag(tag)) {
    NS_LOG_DEBUG(m_name << ss.str() << "  size:" << p->GetSize() << " packet uid: " << p->GetUid() << " time sent: " << tag.GetTime().As(Time::MS));
  } else {
    NS_LOG_DEBUG(m_name << ss.str() << "  size:" << p->GetSize() << " packet uid: " << p->GetUid());
  }

  p->PeekPacketTag(pnt);

  if (t == OTHER && CheckAODVHeader(p)){
    NS_LOG_LOGIC (m_name << p->GetUid() << " is AODV traffic, dont reroute it.");
    return true;
  } else if (t == OTHER && (CheckQLrnHeader(p, qlrnHeader) || CheckQLrnHeader(p, qosQlrnHeader) ) ) {
    NS_LOG_LOGIC (m_name << p->GetUid() << " is QLRN traffic, dont reroute it.");
    return true;
  } else {
    double random_value = m_epsilon->GetValue();
    /*
    1st case : the node is in learning phase (so trying to find a converged optimum) and the packet is learning traffic
    then send to a random next hop 1
    */

    if ( ( random_value < m_eps_thresh) && m_learning_phase[dst] && p->PeekPacketTag(tag) /* &&  pnt.GetLearningPkt() TODO stil lnot sure about htis */ ) {
      NS_LOG_DEBUG(m_name << "Taking an e-greedy decision path for packet " << p->GetUid() << "!. e = " << m_eps_thresh);

      Ipv4Address next_hop = route->GetGateway(); //for log purposes
      p->AddPacketTag(RandomDecisionTag(true));
      auto random_estimate = GetQTable(t).GetRandomEstim(dst);
      initial_estim = random_estimate.GetQValue().GetInteger();
      route->SetGateway(random_estimate.GetNextHop()  );
      NS_LOG_DEBUG(m_name << "I have to randomly replace " << next_hop << " by  "
          << route->GetGateway() << " on packet id " << p->GetUid() << " destined for " << dst );
      // QLrnInfoTag new_tag(Simulator::Now().GetInteger(), m_this_node_ip, tag.GetMaint(),true);
      // p->ReplacePacketTag(new_tag);
    }
    /*
    2nd case : the node is no longer in learing phase and its small learning traffic thats happening
      in that case, [case A] if its at node 0, with a chance of eps_thresh send it to a random next hop and with a chance of 1-eps send it to a random unconverged next hop
      if its not at node 0, [case B] with a chance of rho send the packet to the optimum possible next hop (to help next hop learn) and otherwise
      send it to a random unconverged next hop
    */
    else if ( !m_learning_phase[dst] && pnt.GetLearningPkt() && p->PeekPacketTag(tag) ) { /* so not in learning phase, but it is LEARNING/maintenance traffic*/
      // case A
      if (src == m_this_node_ip || src == Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE) ) {
        // from the source, do the exploration. Intermediate nodes should just try and find their best estimates
        if (src == Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE) ) {
          NS_ASSERT_MSG( std::find(m_traffic_sources.begin(), m_traffic_sources.end(), m_this_node_ip) != m_traffic_sources.end(),
            " we need src to be one of the traffic sources..." << m_this_node_ip) ;
        }
        if (random_value < m_eps_thresh /*(m_eps_thresh == 0 ? 0.5: m_eps_thresh ) */) {// Pick any neighbour : do EXPLORATION ---
          RouteHelper(route,dst,t,initial_estim,0,p); // mode 0 means the next hop will be random
        } else {          // Pick only a non-converged value : do TARGETTED EXPLORATION
          if (GetQTable(t).HasConverged(dst) || AllNeighboursBlacklisted(dst,t)) {
            // seems like this not having the all neighb blacklisted check was the part that was missing for the testmiguel8 issue with vv many learning pkts
            return false; /* drop the packet if there is nothing to learn and pkt wasnt lucky OR all are blacklisted  */
          }
          else { RouteHelper(route,dst,t,initial_estim,1,p); } //mode 1 means the next hop will be an unconverged hop
        }
      }
      // case B
      else { // intermediate nodes (usually) just find their best choice still.
        if (random_value < m_rho) {
          RouteHelper(route,dst,t,initial_estim,2,p); // mode 2 means the next hop will be optimal
        } else {
          // was thinking about having a similar choice here as for node0 but that won't do probably
          // so just make it random ? then at least in some cases itll be the good direction...
          RouteHelper(route,dst,t,initial_estim,0,p); // mode 1 means the next hop will be RANDOM
        }
        // if (random_value > ( m_eps_thresh == 0 ? 0.5: m_eps_thresh ) ) {// Pick any neighbour : do EXPLORATION
        //   Route Helper(route,dst,t,initial_estim,false,p);
        // } else {          // Pick only a non-converged value : do EXPLORATION
      }
    } else if (!m_learning_phase[dst] && (!pnt.GetLearningPkt() && p->PeekPacketTag(tag) ) ) {
      std::stringstream ss;p->Print(ss<<std::endl);
    } else if (!m_learning_phase[dst] && (pnt.GetLearningPkt() && !p->PeekPacketTag(tag) ) ) {
      std::stringstream ss;p->Print(ss<<std::endl<<p->GetUid()<<"   ");
      if (t != ICMP ) { NS_LOG_UNCOND("This is odd, what packet was it ?: \n" << ss.str() << "==message over=="); }
      if (t != ICMP) { } else { NS_FATAL_ERROR(ss.str() << "\nTest for unreachable code : learning packet without QLrnTag header");}
    }
    /*
    case 3
    */else {
    Icmpv4TimeExceeded ii;
      // back to here
      /**
       * Its REAL traffic or we are in learning phase and the traffic got a good RNG roll so it was above eps threshold ,
       * and is intended to explore the currently-optimal solution. In any case, we should make the (perceived) optimal routing decision
       **/
      auto next_estimate = GetQTable(t).GetNextEstim(dst);
      // so this is for the TO-DO show that A/B/C  are taking different paths so they each have a QoS set
      // it will only return the a_estim for b if there is no non-blacklisted alternative, same with b_estim for c
      // p->Print(std::cout<<std::endl);std::cout << p->GetUid() << "  " << t << std::endl;

      if (!pnt.GetLearningPkt()) {
        RouteDiffBasedOnType(next_estimate, dst, src, t);
      }

      NS_LOG_DEBUG(m_name << "(Traffictype = " << t << ")  I want to replace " << route->GetGateway() << " by  "
                     << next_estimate.GetNextHop() << " on packet id " << p->GetUid() << " destined for " << dst << " because "
                     << next_estimate.GetQValue() << " is the lowest value I found in my qtable (for dst = " << dst << ")." << "\n"
                     << /*GetQTable(t).PrettyPrint()*/"" << " PS BE CAREFUL IF ALL BLACKLISTED THIS WILL BE A RANDOM RESULT TOO.");

      if (!pnt.GetLearningPkt() && !p->PeekPacketTag(tag) && GetPacketTable()->GetNumberOfTimesSeen(p->GetUid()) > m_max_retry ) {
        // for this max retry parameter, if we set it to 0, it will work on the 1st time we see the pkt again (i think)
        // Try to let the QLearner decide for itself to learn more and figure a way out of the loop
        tag.SetTime(Simulator::Now().GetInteger());
        tag.SetPrevHop(m_this_node_ip);
        tag.SetMaint(true); // false means its dropped immediately at the next hop, clearly this wont do! must be _true_
        tag.SetUsableDelay(false);


        p->AddPacketTag(tag);
        // p->Print(std::cout<<std::endl);std::cout << p->GetUid() << std::endl;
        // NS_LOG_UNCOND(  m_name << " is adding a lrn header to " << p->GetUid() << " because it has been seen more than max_retry times. \n"
        //              << GetQTable(t).PrettyPrint() << "\n");

      } else if (!p->PeekPacketTag(tag) && !pnt.GetLearningPkt() && GetAODV()->CheckIcmpTTLExceeded(p,ii)) { // because of adding this call to icmp ttl exceeded we observed issue
        return false; // -- this isnt going to benefit anyyone, drop the icmp ttl packet instead of waterproofing the adding of a QLrnInfoTag to it, as it goes wrong SOMEWHERE
      }

      initial_estim = next_estimate.GetQValue().GetInteger();
      route->SetGateway( next_estimate.GetNextHop() );
    }

    if (!p->PeekPacketTag(ptab) && !p->PeekPacketTag(ptct) && !p->PeekPacketTag(ptst_tag) ) {
      ptab.SetEstimTypeA(GetQTable(TRAFFIC_A).GetNextEstim(dst).GetQValue().GetInteger());
      ptab.SetEstimTypeB(GetQTable(TRAFFIC_B).GetNextEstim(dst).GetQValue().GetInteger());
      ptct.SetEstimTypeC(GetQTable(TRAFFIC_C).GetNextEstim(dst).GetQValue().GetInteger());

//      NS_LOG_UNCOND("set the time in the PTCT: the precursor for the eventual tag");
      ptct.SetSentTime(Simulator::Now().GetInteger());

      p->AddPacketTag(ptab);
      p->AddPacketTag(ptct);
    }
  }

  // Since the packet is being sent here no matter what (added assert for loopback just to be sure)
  // We should clear it from the packet info
  NS_ASSERT_MSG(route->GetGateway() != Ipv4Address(LOCALHOST), "After QLearner::Route we dont want route to be loopback");
  // m_packet_info.ClearPacket(p->GetUid()); -- incorrect, probably gonna put this in wifi-net-device

  if (route->GetGateway() == Ipv4Address(NO_NEIGHBOURS_REACHABLE_ROUTE_IP) && initial_estim == MilliSeconds(NO_NEIGHBOURS_REACHABLE_ROUTE_MS)) {
    return false;
  }
  if (!pnt.GetLearningPkt()) {
    RoutingPacketViaNeighbToDst(route->GetGateway(), route->GetDestination());
  }
  return true;
}

void QLearner::RouteDiffBasedOnType(QTableEntry& next_estimate, Ipv4Address dst, Ipv4Address src, TrafficType t) {
  return; // OFF-switch for this thing if you want it, no point causing extra weird behaviour right
  if (src != m_this_node_ip && src != Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE) ) { return; }
  // if (src != Ipv4Address("10.1.1.1") && src != Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE) ) { return; }
  QTableEntry a_estim;
  QTableEntry b_estim;
  // auto old_adr = next_estimate.GetNextHop();
  QTableEntry next_estimate_by_ref = next_estimate;
  if (next_estimate.GetNextHop() != Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE) ) {
    next_estimate_by_ref = GetQTable(t).GetEntryByRef(dst, next_estimate.GetNextHop());
  }


  if (!AllNeighboursBlacklisted(dst,t)) {
    if (t == TRAFFIC_A) { /* leave the best choice, this is most important */}
    else if (t == TRAFFIC_B) {
      // find the best estim NOT EQUAL to the optimal estimate
      // std::cout << next_estimate_by_ref.GetRealLoss()<< "  "<< 1-(next_estimate_by_ref.GetRealLoss()/10000.0) << "  "
      //   << TrafficTypeReqsMap[TRAFFIC_A].GetRandomLossMax() << "  " << next_estimate_by_ref.GetNextHop()<< std::endl;

      if (1-(next_estimate_by_ref.GetRealLoss()/10000.0) > TrafficTypeReqsMap[TRAFFIC_A].GetRandomLossMax() &&
             next_estimate.GetNextHop() != Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE) &&
             next_estimate_by_ref.GetRealLoss() != 0) {
        // could fill in here to get the 2nd best estimate, but thats not very interesting. We're mainly interested in seeing B take the faster option!
        std::cout << "-" << next_estimate_by_ref.GetRealLoss()<< "  "<< 1-(next_estimate_by_ref.GetRealLoss()/10000.0) << "  "
          << TrafficTypeReqsMap[TRAFFIC_A].GetRandomLossMax() << "  " <<  next_estimate_by_ref.GetNextHop() << std::endl;
      } else {
        a_estim = next_estimate;
      }
      // NS_LOG_UNCOND("trying to get next hop for b that avoids " << a_estim.GetNextHop()
        // << GetQTable(t).PrettyPrint("try"));
      next_estimate = GetQTable(t).GetNextEstim(dst,a_estim.GetNextHop(),Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE));
      // NS_LOG_UNCOND("got " << next_estimate.GetNextHop());
    } else if (t == TRAFFIC_C) {
      // find the best estim that is not equal to the estim for A
      a_estim = next_estimate;
      // NS_LOG_UNCOND("trying to get next hop for b that avoids " << a_estim.GetNextHop());
      b_estim = GetQTable(t).GetNextEstim(dst,a_estim.GetNextHop(),UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE); //duh, using wrong version of teh fct
      // NS_LOG_UNCOND("got " << b_estim.GetNextHop());
      next_estimate = GetQTable(t).GetNextEstim(dst,a_estim.GetNextHop(),b_estim.GetNextHop());
    } else {
      // leave it I suppose
      // ttl exceeded comes here unfortunately
      // NS_FATAL_ERROR("unreachable code, ideally.");
    }
  }
  // NS_LOG_UNCOND("Traffic type is " << traffic_type_to_traffic_string(t) << ". Changed next hop from "
  //   << old_adr << " to " << next_estimate.GetNextHop() << " at " << m_name << "\nI have to avoid "
  //   << a_estim.GetNextHop() << " and " << b_estim.GetNextHop() << "\n" << GetQTable(t).PrettyPrint());
}

void QLearner::RoutingPacketViaNeighbToDst(Ipv4Address via,Ipv4Address dst) {
  // find newest entry ...
  std::tuple<uint64_t, uint64_t, float> most_recent = std::tuple<uint64_t, uint64_t, float>(0,0,0);
  for (const auto& i : m_packet_sent_stats_per_neighb_per_time[std::pair<Ipv4Address,Ipv4Address>(via,dst)]) {
    if (std::get<0>(i) > std::get<0>(most_recent)) {
      most_recent = i;
    }
  }

  std::tuple<uint64_t, uint64_t, float> new_value =
    std::tuple<uint64_t, uint64_t, float>(Simulator::Now().GetInteger(), std::get<1>(most_recent)+1, std::get<2>(most_recent));

  m_packet_sent_stats_per_neighb_per_time[std::pair<Ipv4Address,Ipv4Address>(via,dst)].push_back(new_value);
  CleanTheNumPktsTimestampMap((Simulator::Now()-Seconds(5)).GetInteger(), via, dst);
}

void QLearner::CleanTheNumPktsTimestampMap(uint64_t time_cutoff,Ipv4Address via,Ipv4Address dst) {
  bool done = false;
  while (!done) {
    done = true;
    for (unsigned int i = 0; i < m_packet_sent_stats_per_neighb_per_time[std::pair<Ipv4Address,Ipv4Address>(via,dst)].size(); i++ ) {
      if (std::get<0>(m_packet_sent_stats_per_neighb_per_time[std::pair<Ipv4Address,Ipv4Address>(via,dst)].at(i)) < time_cutoff &&
          m_packet_sent_stats_per_neighb_per_time[std::pair<Ipv4Address,Ipv4Address>(via,dst)].size() != 1) {
        m_packet_sent_stats_per_neighb_per_time[std::pair<Ipv4Address,Ipv4Address>(via,dst)].erase(
          m_packet_sent_stats_per_neighb_per_time[std::pair<Ipv4Address,Ipv4Address>(via,dst)].begin()+i);
        done = false;
        break;
      }
    }
  }
}

uint64_t QLearner::GetNumPktsSentViaNeighbToDst(uint64_t ts, Ipv4Address via, Ipv4Address dst) {
  std::tuple<uint64_t, uint64_t, float> ret = std::tuple<uint64_t, uint64_t, float>(0,0,0);

  for (const auto& i : m_packet_sent_stats_per_neighb_per_time[std::pair<Ipv4Address,Ipv4Address>(via,dst)]) {
    if (std::get<0>(i) < ts && std::get<0>(i) > std::get<0>(ret)) {
      ret = i;
    }
  }
  return std::get<1>(ret);
}

void QLearner::ReceivedPktFromPrevHopToDst(Ipv4Address prev_hop, Ipv4Address dst) {
  // if (GetNode()->GetId() == 2) {
  //   std::cout << m_this_node_ip << "  received  " << m_packet_recv_stats_per_neighb[std::pair<Ipv4Address,Ipv4Address>(prev_hop,dst)].first
  //   << "  from  " << prev_hop
  //   <<  "   to   " << dst << std::endl;
  // }
  m_packet_recv_stats_per_neighb[std::pair<Ipv4Address,Ipv4Address>(prev_hop,dst)].first += 1;
}

uint64_t QLearner::GetNumRecvPktFromPrevHopToDst(Ipv4Address prev_hop, Ipv4Address dst) {
  return m_packet_recv_stats_per_neighb[std::pair<Ipv4Address,Ipv4Address>(prev_hop,dst)].first;
}

void QLearner::SetRealLossForOutput(Ipv4Address prev_hop, Ipv4Address dst, float new_value) {
  m_packet_recv_stats_per_neighb[std::pair<Ipv4Address,Ipv4Address>(prev_hop,dst)].second = new_value;
}

void QLearner::HandleRouteOutput(Ptr<Packet> p, const Ipv4Header &header, TrafficType t) {
  Ipv4Address bcast        = GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetBroadcast();

  NS_ASSERT_MSG (Time::GetResolution () == 7, "As defined in nstime.h, Unit value should be 7 == NS. Undefined behaviour if this condition is not satisfied.");

  QLrnInfoTag tag;
  PortNrTag pnt;
  p->PeekPacketTag(pnt);
  if (!p->PeekPacketTag(tag)  && header.GetDestination()!=bcast) {
    /**
     *  No QLrnInfoTag was found, we are QLearning and this is the RouteOutput function, so we are also the source.
     *  To check this, following assert:
     */

    NS_ASSERT_MSG(header.GetSource() == m_this_node_ip || header.GetSource() == Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE), "We are the source when its route output");
    /**
     *  Sometimes source hasnt been set yet, then it is equal to UNINITIALIZED_IP_ADDRESS_VALUE,
     *  Next, we find the qlrnHeader if there is one, we cause a flood of the network if we reply to QInfo packets with other QInfo packets, clearly.
     */
    QLrnHeader qlrnHeader;
    QoSQLrnHeader qosQlrnHeader;
    if ( CheckQLrnHeader(p, qlrnHeader) || CheckQLrnHeader(p, qosQlrnHeader) ) {
      /* So if the packet is a QInfo packet, dont add a tag because then we will be getting more QInfo packets in response, and these packets are always from next-hop neighbours anyway */
      NS_LOG_DEBUG("(RouteOutput)" << m_name << "Not adding a tag to packet " << p->GetUid() << " because it is a QInfo packet. PrevHop of QInfo: "  << tag.GetPrevHop());
    } else if (CheckAODVHeader(p)) { //disable learning off AODV packets here if you want
        tag.SetTime(Simulator::Now().GetInteger());
        tag.SetPrevHop(m_this_node_ip);
        NS_LOG_DEBUG("[RouteOutput node " << GetNode()->GetId() << "] : Adding a tag to packet " << p->GetUid() << " (which has size = " << p->GetSize() << ")   PrevHop:"  << tag.GetPrevHop()<< ". This packet is meant for " << header.GetDestination());
        tag.SetMaint(!m_learning_phase[header.GetDestination()]);
        p->AddPacketTag(tag);
    } else {
      // careful here, icmp must be in lrn phase both directions
      if ((m_learning_phase[header.GetDestination()] && pnt.GetLearningPkt() ) || pnt.GetLearningPkt() || (m_learning_phase[header.GetDestination()] && m_num_applications <= 1 ) /* added for legacy reasons, if only 1 application (icmp test21/22)*/ ){
        /* _any_ other packet, then we do add a PTST tag, because we do want to know the next hop's estimate. */
        NS_LOG_DEBUG("(RouteOutput)" << m_name << "Adding a tag to packet " << p->GetUid() << " (which has size = " << p->GetSize() << ")   PrevHop:"  << tag.GetPrevHop()<< ". This packet is meant for " << header.GetDestination() << " and is being sent at " << Simulator::Now().As(Time::MS));
        tag.SetTime(Simulator::Now().GetInteger());
        tag.SetPrevHop(m_this_node_ip);
        tag.SetMaint(!m_learning_phase[header.GetDestination()]);
        p->AddPacketTag(tag);
      } else {
        // Add a tag signifying that it is a packet being handled by QRouting!
        QRoutedTrafficPacketTag q(true);
        p->AddPacketTag(q);
      }
    }
  } else if (p->PeekPacketTag(tag)) {
    /**
     * If we see a packet that already has a tag in this RouteOutput function we are not sure what has happened to allow that.
     * It should always be the source that has RouteOutput called, and from then on it should be the RouteInput determining the plays
     * [[ special case = DeferredRouteOutput but this is handled in RouteInput ]]
     */
    NS_ASSERT_MSG(false, "Packet with tag already present in RouteOutput! This should not happen.");
  }
}

void QLearner::HandleRouteInput(Ptr<Packet> p, const Ipv4Header &header, bool deferred, bool& random_sentback, TrafficType t) {
  //Route an input packet (to be forwarded or locally delivered)
  Ipv4Address bcast        = GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetBroadcast();

  NS_ASSERT_MSG (Time::GetResolution () == 7, "As defined in nstime.h, Unit value should be 7 == NS. Undefined behaviour if this condition is not satisfied.");

  QLrnInfoTag tag;

  // tried to do it in Route fct, but cant remove there
  RandomDecisionTag rd_tag;
  random_sentback = p->RemovePacketTag(rd_tag);

  PacketTimeSentTagPrecursorAB ptab;
  PacketTimeSentTagPrecursorCT ptct;

  PacketTimeSentTag ptst_tag_2; p->PeekPacketTag(ptst_tag_2);
  PortNrTag pnt; p->PeekPacketTag(pnt);

  if (p->PeekPacketTag(ptct) && p->PeekPacketTag(ptab) ) {
    PacketTimeSentTag ptst;

    if      (t == VOIP  || t == TRAFFIC_A) { ptst.SetInitialEstim(ptab.GetEstimTypeAAsInt()); }
    else if (t == VIDEO || t == TRAFFIC_B) { ptst.SetInitialEstim(ptab.GetEstimTypeBAsInt()); }
    else                                   { ptst.SetInitialEstim(ptct.GetEstimTypeCAsInt()); }

//    NS_LOG_UNCOND("set the time in the PTST: Packet Time Sent Tag");
    ptst.SetSentTime(ptct.GetSentTimeAsInt());
    ptst.SetPrevHop(m_this_node_ip);
    p->RemovePacketTag(ptab);
    p->RemovePacketTag(ptct);

    p->AddPacketTag(ptst);
    if (header.GetDestination() != bcast && p->PeekPacketTag(pnt) && !pnt.GetLearningPkt() ) {
      ReceivedPktFromPrevHopToDst(header.GetSource(),header.GetDestination()); //must be 1st hop or the ptab,ptct tags would be gone
    }
  } else if (header.GetDestination() != m_this_node_ip) {
    PacketTimeSentTag ptst_tag;
    p->RemovePacketTag(ptst_tag);
    if (header.GetDestination() != bcast && p->PeekPacketTag(pnt) && !pnt.GetLearningPkt() ) {
      ReceivedPktFromPrevHopToDst(ptst_tag.GetPrevHop(),header.GetDestination()); //must be 1st hop or the ptab,ptct tags would be gone
      ptst_tag.SetPrevHop(m_this_node_ip);
      p->AddPacketTag(ptst_tag);
    }
  } else if ( p->PeekPacketTag(ptst_tag_2) ){
    if (header.GetDestination() != bcast && p->PeekPacketTag(pnt) && !pnt.GetLearningPkt() ) {
      ReceivedPktFromPrevHopToDst(ptst_tag_2.GetPrevHop(),header.GetDestination()); //must be 1st hop or the ptab,ptct tags would be gone
    }
  }

  if (!p->PeekPacketTag(tag) && header.GetDestination() != m_this_node_ip && header.GetDestination()!=bcast) {

    /**
      * If we reach this part of the code, then we arrive in a situation where a packet arrives for which we are not the source
      * that does not have a QLrnInfoTag.
      *
      * This may be fine in the case that it is a QInfo packet, or if it is a ACK / RTS / CTS ?
      * but then why were there no errors before
      */
      TcpHeader tcpHdr;
      QLrnHeader qlrnHeader;
      QoSQLrnHeader qosQlrnHeader;
      if ( CheckQLrnHeader(p, qlrnHeader) || CheckQLrnHeader(p, qosQlrnHeader) ) {
        // std::cout << "QLrnHeader found!\n"; // so this one is fine
      } else if (true) {
        if (!CheckDestinationKnown(header.GetDestination()) ){
          NS_LOG_DEBUG (m_name << "ADD_DST_CASE_9  ---  dst " << header.GetDestination() << " via " << tag.GetPrevHop() << "  " );
          AddDestination(Ipv4Address(IP_WHEN_NO_NEXT_HOP_NEIGHBOUR_KNOWN_YET), header.GetDestination(), Seconds(10));
          DropPacketTag dpt(true);
          p->AddPacketTag(dpt);
          aodvProto->SendRequest(header.GetDestination());
        }
      } else if ( p->PeekHeader (tcpHdr) ) { //way too easy to pass this
        tcpHdr.Print(std::cout << "tcpHdr found!\n");std::cout << " " << (tcpHdr.IsChecksumOk() ? "  -- checksum is FINE" : "CHECK SUM IS NOT FINE!")<< "\n";
        // TCP PACKET
      } else {
        p->Print(std::cout);std::cout<<std::endl;
        NS_ASSERT_MSG(false, "Found no PTST tag even though it is not a QInfo packet and it is not a bcast packet and not a TCP packet, also we are not the destination. ");
      }
  } else if (p->PeekPacketTag(tag) && header.GetSource() == m_this_node_ip ) {

    /**
     * After all, who would have added the tag?
     * --> There are some cases where that will happen
     * i.e. a bcast - deferred route output (772) - random decision - bad decision -
     */

    if (deferred) {
      NS_LOG_DEBUG( "(RouteInput)" << m_name << GetNode()->GetId() << "] : "<< "DEFERREDOUTPUTTAG   " << p->GetUid() << " DEFERRED = TRUE" << std::endl );
      /*
       * Ok so apparently this is sometimes fine and expected behaviour, that it passes RouteOutput then once more through RouteInput, I dont know.
       * Deferred routing lets it flow past the same interface twice, the 2nd time means it passes RouteInput...
       */
    } else {
      NS_LOG_DEBUG( "(RouteInput)" << m_name << GetNode()->GetId() << "] : "<< "DEFERREDOUTPUTTAG   " << p->GetUid() << " DEFERRED = FALSE" << std::endl );
      if (header.GetDestination() == bcast ) {
        /**
         * More edge case.. a broadcast message is also received by the sender
         * Since its bcast, dont do anything with it
         */
      } else if ( rd_tag.GetRandomNextHop() ) {
        // GetRandomNextHop is a boolean only, i.e. if the packet was randomly sent back and we (as the source) pick it up, this happens
        // since it should be unicast (right?) this wont cause duplicate packets to occur i guess
        NS_LOG_DEBUG( "(RouteInput)" << m_name << "We saw a packet (" << p->GetUid() << " ) for which we are the source due to random (e-greedy) routing decision.");
        NS_LOG_DEBUG("(RouteInput)" << m_name << "Found a tag on packet " << p->GetUid() << ". Prev hop = " << tag.GetPrevHop() << ". Replacing it & sending back some info.") ;

        QLrnInfoTag new_tag(Simulator::Now().GetInteger(), m_this_node_ip, tag.GetMaint(),tag.GetUsableDelay());
        /* Take care of sending back a packet to the previous hop (found in tag->GetPrevHop() ) */
        // p->Print(std::cout);std::cout<<std::endl;
        // if (true) {std::stringstream ss; p->Print(ss<<std::endl<<"(D)Node"<<GetNode()->GetId()<< "Sending info back because of this packet:");ss<<std::endl;std::cout<<ss.str();}
        bool sender_convergence = !CheckAODVHeader(p) && GetQTable(t).HasConverged(header.GetDestination() , true);

        // std::cout << p->GetUid() << " at (B)" << m_name << std::endl;
        Send(tag.GetPrevHop(), p->GetUid(), TravelTimeHelper(tag), header.GetDestination(), header.GetSource(), t, sender_convergence ) ;
        if (GetPacketTable()->GetNumberOfTimesSeen(p->GetUid()) > 1) {       new_tag.SetUsableDelay(false);     }
        p->ReplacePacketTag(new_tag);
      } else if ( !random_sentback) {
        QLrnInfoTag new_tag(Simulator::Now().GetInteger(), m_this_node_ip, tag.GetMaint(), tag.GetUsableDelay());
        NS_LOG_DEBUG("(RouteInput)" << m_name << "Found a tag on packet i alrdy sent once, pid = " << p->GetUid() << ". Prev hop = " << tag.GetPrevHop() << ". Replacing it & sending back some info.") ;

        /* Take care of sending back a packet to the previous hop (found in tag->GetPrevHop() ) */
        // p->Print(std::cout);std::cout<<std::endl;
        // if (true) {std::stringstream ss; p->Print(ss<<std::endl<<"(A)Node"<<GetNode()->GetId()<< "Sending info back because of this packet:");ss<<std::endl;std::cout<<ss.str();}
        bool sender_convergence = !CheckAODVHeader(p) && GetQTable(t).HasConverged(header.GetDestination() , true);
        // std::cout << p->GetUid() << " at (C)" << m_name << std::endl;
        Send(tag.GetPrevHop(), p->GetUid(), TravelTimeHelper(tag), header.GetDestination(), header.GetSource(), t, sender_convergence ) ;
        if (GetPacketTable()->GetNumberOfTimesSeen(p->GetUid()) > 1) {       new_tag.SetUsableDelay(false);     }
        p->ReplacePacketTag(new_tag);
      } else {
        std::stringstream ss; p->Print(ss); NS_LOG_DEBUG( "(RouteInput)" << m_name << ss.str());
        NS_ASSERT(false);
      }
    }

  } else if (p->PeekPacketTag(tag) && header.GetSource() != m_this_node_ip && header.GetDestination() != m_this_node_ip) {

    /**
     * QLrnInfoTag was found and there is a m_qlearner
     * Added to this fact is that we are not the destination of the packet and we are not the source
     * we must send back QInfo to the previous hop and also forward the packet as we normally would, this means updating the tag!
     */
    NS_ASSERT_MSG (header.GetSource() != m_this_node_ip, "[Works as long as 1 interface per node] Source of pkt should not be finding a PTST already present");
    QLrnInfoTag new_tag(Simulator::Now().GetInteger(), m_this_node_ip, tag.GetMaint(),tag.GetUsableDelay());
    NS_LOG_DEBUG("(RouteInput)" << m_name << "Found a tag on packet " << p->GetUid() << ". Prev hop = " << tag.GetPrevHop() << ". Replacing it & sending back info.") ;

    /* Take care of sending back a packet to the previous hop (found in tag->GetPrevHop() ) */
    // p->Print(std::cout);std::cout<< "   pktId : " << p->GetUid()<<std::endl;
    /*
     * Noticed issue in test5:
     *  when a hop gets a packet from the source and its not on the AODV Route then it wont know about the destination
     *  So what should happen in that case (imo) is give a Q feedback pkt with a high estimate(?),
     *  send the packet back to src s.t. another route can be found and send a RREQ for the real dst
     *
     *
     * I guess technically this becomes ADD_DST_CASE_7
     */
    if (!CheckDestinationKnown(header.GetDestination()) ){
      NS_LOG_DEBUG (m_name << "ADD_DST_CASE_7  ---  dst " << header.GetDestination() << " via " << tag.GetPrevHop() << "  " );
      AddDestination(tag.GetPrevHop(), header.GetDestination(), Seconds(10));
      aodvProto->SendRequest(header.GetDestination());
    }

    bool sender_convergence = !CheckAODVHeader(p) && GetQTable(t).HasConverged(header.GetDestination() , true);
    // if (GetNode()->GetId() >= 1 && m_travel_time > MilliSeconds(20)) {
    //   std::stringstream ss; p->Print(ss<<std::endl<<"(B)Node"<<GetNode()->GetId()<< "Sending info back because of this packet:");ss<<std::endl;std::cout<<ss.str()
    //   << (p->PeekPacketTag(tag)?"Does have a qlrn tag": "does not have a qlrn tag") << "     " << m_travel_time << "\n";
    // }
    // if (true) {std::cout << " ptst prev hop = " << tag.GetPrevHop() << " and we are " << m_name << std::endl;p->Print(std::cout<<std::endl);std::cout<<std::endl
    //   << header.GetSource() << "  " << p->GetUid() << std::endl;}
    // p->Print(std::cout << p->GetUid() << " at " << m_name << "      ");std::cout << std::endl;
    Send(tag.GetPrevHop(), p->GetUid(), TravelTimeHelper(tag), header.GetDestination(), header.GetSource(), t, sender_convergence ) ;

    if (GetQTable(t).HasConverged(header.GetDestination(),true) && m_use_learning_phases/* for legacy reasons */ && !tag.GetMaint() ) {
      /* !tag.GetMaint() is for if its a maintenance packet (to keep learning while traffic) dont drop*/

      // Its already determined here that the tag is there from the else if from before, now we just drop if we're converged
      // the m_use_learning_phases is added for tests with ICMP
      // the m_num_applications was added for test 49 (and others, probably) : after 42 packets node1 would C and stop forwarding thus not getting the 49 expected pkts -- but not correct bc node1 never has appl
      // just changed it to 42 instead of 49, can change it but meh who cares eh
      p->RemovePacketTag(tag);
      DropPacketTag dpt(true);
      p->AddPacketTag(dpt);
    } else {
      if (GetPacketTable()->GetNumberOfTimesSeen(p->GetUid()) > 1) {       new_tag.SetUsableDelay(false);     }
      p->ReplacePacketTag(new_tag);
    }
  } else if (p->PeekPacketTag(tag) && header.GetDestination() == m_this_node_ip && (t != OTHER || CheckAODVHeader(p)) ){
    /*TODO ICMP TTL EXCeeded handling should happen here I suppose, not sure */
    /**
     * QLrnInfoTag was found and there is a m_qlearner
     * Added to this fact is that we are the destination of the packet and we are not the source (though we never can be the source)
     * we must send back QInfo to the previous hop and not forward the packet so we dont update the tag
     */
    // NS_LOG_UNCOND ("(RouteInput)" << m_name << "Found a tag on packet " << p->GetUid() << ". Prev hop was " << tag.GetPrevHop() << ". ACTUALLY NOT Removing it & trying to send back some info. CurrTime:" << Simulator::Now().As(Time::MS) );
    // std::stringstream ss; p->Print(ss<<std::endl);
    // NS_LOG_UNCOND(ss.str());

    /*GetQTable(t).HasConverged(header.GetDestination() ) we are dst, so this automatically true*/
    bool sender_convergence = !CheckAODVHeader(p) && true;
    // dont cause other nodes to converge as a result of aodv traffic please

    /* Take care of sending back a packet to the previous hop (found in tag->GetPrevHop() ) */
    // p->Print(std::cout);std::cout<< "   pktId : " << p->GetUid()<<std::endl;
    // p->Print(std::cout);std::cout<<std::endl;
    // if (true) { std::stringstream ss; p->Print(ss<<std::endl<<"(C)Node"<<GetNode()->GetId()<< "Sending info back because of this packet (id = "<< p->GetUid() <<") :  ");ss<<std::endl;std::cout<<ss.str(); }
    // std::cout << p->GetUid() << " at (A) " << m_name << std::endl;
    Send(tag.GetPrevHop(), p->GetUid(), TravelTimeHelper(tag), header.GetDestination(), header.GetSource(), t, sender_convergence  ) ;

    /** Wouldve liked to remove the tag just as a way of being clean, but unfortunately this is not possible.
     *  The tag is needed in the local delivery to the AODV underlying protocol (only that one, but maybe more later)
     *  So commented this out for now
     */
      // p->RemovePacketTag(tag);

    PacketTimeSentTag ptst_tag;
    if (p->PeekPacketTag(ptst_tag) && m_use_learning_phases) {
      // p->Print(std::cout<<std::endl); std::cout << p->GetUid() << std::endl;
      if (m_report_dst_to_src) {
        NS_FATAL_ERROR("Not using this anymore, need unless you specify who the dst is in the function this wont work anymore (which is possbile, header.GetDst, but worth?)");
        // m_other_qlearners[header.GetSource()]->ReportReceivedPacketMetrics(Simulator::Now() - ptst_tag.GetSentTime(),
        //   ptst_tag.GetInitialEstim(),
        //   p->GetUid(),
        //    true /* learner_traffic */
        //  );
      }
    }
    PortNrTag pnt;
    // Had to add this to make sure its actually traffic of ours.. If there is a PNT, it came from one of the onoff appl we configured
    // and we can then reasonably decide if its learning or real traffic
    if (m_this_node_ip == /*m_traffic_destination*/ header.GetDestination() && p->PeekPacketTag(pnt))  {
      // but its due to the first packet (its not seperate learning, traffic, so thats why)
      // p->Print(std::cout << m_name << " to check why its suddenly really big delay" << "  " );std::cout<< "  " << m_running_avg_latency.first << " " << m_running_avg_latency.second <<std::endl;
      // std::stringstream ss;p->Print(ss);ss<<std::endl;NS_LOG_UNCOND(ss.str());
      UpdateAvgDelay(ptst_tag,pnt);
      if (m_output_data_to_file) {
        // so this bit is a little strange.. the packets that get re-labeled for learning cause huge spikes!
        OutputDataToFile(ptst_tag, p, pnt.GetLearningPkt() /* due to the anti-loop system, this is needed */, t, header.GetSource() );
      }
    }
  } else if (!p->PeekPacketTag(tag) && header.GetDestination() == m_this_node_ip) {
    // this clause added for the case where its not learning traffic and we still want to check our metrics
    PacketTimeSentTag ptst_tag;
    if (p->PeekPacketTag(ptst_tag) && m_use_learning_phases) {
      aodvProto->CheckTraffic(p,t);
      if (t == OTHER) { } //See other TODO for icmp ttl exc above
      else {
        if (m_report_dst_to_src) {
          NS_FATAL_ERROR("Not using this anymore, need unless you specify who the dst is in the function this wont work anymore (which is possbile, header.GetDst, but worth?)");
          // m_other_qlearners[header.GetSource()]->ReportReceivedPacketMetrics(Simulator::Now() - ptst_tag.GetSentTime(),
          //   ptst_tag.GetInitialEstim(),
          //   p->GetUid(),
          //   false /* learner_traffic */
          // );
        }
      }
    }

    PortNrTag pnt;
    // Had to add this to make sure its actually traffic of ours.. If there is a PNT, it came from one of the onoff appl we configured
    // and we can then reasonably decide if its learning or real traffic.. ?
    if (m_this_node_ip == /*m_traffic_destination*/ header.GetDestination() && p->PeekPacketTag(pnt) )  {
      // std::cout << m_this_node_ip << std::endl;
      // but its due to the first packet (its not seperate learning, traffic, so thats why)
      // p->Print(std::cout << m_name << " to check why its suddenly really big delay" << "  " );std::cout<< "  " << m_running_avg_latency.first << " " << m_running_avg_latency.second <<std::endl;
      UpdateAvgDelay(ptst_tag,pnt);
      if (m_output_data_to_file) {
       OutputDataToFile(ptst_tag, p, false /* no QLrnInfoTag means its auto false */ , t , header.GetSource() );
      }
    }
  }
}

// void QLearner::ReportReceivedPacketMetrics(Time real_time, Time expected_time, uint64_t pktID, bool learner_traffic) {
//   // For the calculation, one idea : https://www.calculatorsoup.com/calculators/algebra/percent-change-calculator.php
//   // another :                       https://www.calculatorsoup.com/calculators/algebra/percent-difference-calculator.php
//   // But does not matter that much I guess?
//   float percent_change = std::abs(float(expected_time.GetInteger() - real_time.GetInteger())) / real_time.GetInteger();
//   // std::cout << "real time: " << real_time.As(Time::S) << " exp time:  " << expected_time << " percent change  " << percent_change << "  thresh: " << m_learning_threshold << std::endl;
//   if (!m_learning_phase[""] && !learner_traffic && percent_change > m_learning_threshold) {
//     NS_LOG_DEBUG( "Real value was " << real_time << " and initial estimate was " << expected_time );
//     NS_LOG_UNCOND( "node" << GetNode()->GetId() << " getting reports back to learn more: pkt id:" << pktID << " time: "
//                           << Simulator::Now().As(Time::S) << " percent_change = " << percent_change << " expected time: " << expected_time.As(Time::S) << "  and real time:  " << real_time.As(Time::S));
//     StopTrafficStartLearning();
//   } else if (m_learning_phase && learner_traffic && percent_change > m_learning_threshold) {
//     NS_LOG_DEBUG( "I am node" << GetNode()->GetId() << " and Im getting some reports back that I should learn some more, but I already am!");
//     //nothing else happens, m_learning_phase is already set to true, so it is already learning
//   } else if (percent_change <= m_learning_threshold) {
//     NS_LOG_DEBUG( "I am node" << GetNode()->GetId() << " and Im getting some reports back that I should not learn more");
//     //nothing else happens, un-setting m_learning_phase happens elsewhere when differences between old and new values are observed to be small
//   } else if (m_learning_phase && !learner_traffic /*&& percent_change > m_learning_threshold this doesnt matter*/ ) {
//     NS_LOG_UNCOND("Not doing anything. I am in learning phase and received non-learning traffic in pkt " << pktID << ", this is at time t = " << Simulator::Now().As(Time::S));
//   } else if (learner_traffic && !m_learning_phase /*&& percent_change > m_learning_threshold this doesnt matter*/ )  {
//     NS_LOG_UNCOND("Not doing anything. I am not in my learning phase and I received learning traffic in pkt " << pktID << ", this is at time t = " << Simulator::Now().As(Time::S));
//   } else {
//     NS_FATAL_ERROR("theres no way this is reachable");
//   }
// }


bool QLearner::CheckQLrnHeader_withoutUDP(Ptr<const Packet> p, QoSQLrnHeader& qlrnHeader) {
  return false;
}
bool QLearner::CheckQLrnHeader(Ptr<const Packet> p, QoSQLrnHeader& qlrnHeader) {
  return false;
}
bool QLearner::CheckQLrnHeader_withoutUDP(Ptr<const Packet> p, QLrnHeader& qlrnHeader) {
  p->PeekHeader(qlrnHeader);
  NS_ASSERT(qlrnHeader.IsValid());
  return qlrnHeader.IsValid();
}

bool QLearner::CheckQLrnHeader(Ptr<const Packet> p, QLrnHeader& qlrnHeader) {
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

bool QLearner::CheckAODVHeader(Ptr<const Packet> p) {
  Ptr<Packet> p_copy = p->Copy();

  /**
   * another workaround for tests ...
   * this time, due to SeqTsHeader being seen as a AODV header
   * weird
   */
  if (p->GetSize() == TEST_UDP_NO_ECHO_PKT_SIZE) {
    return false;
  }

  aodv::TypeHeader tHeader (aodv::AODVTYPE_RREP_ACK);
  UdpHeader u;

  p_copy->RemoveHeader(u);
  p_copy->RemoveHeader (tHeader);

  if (tHeader.IsValid()) {
    switch (tHeader.Get ()) {
      case aodv::AODVTYPE_RREQ :
      {
        NS_LOG_LOGIC("Message found was an AODV::RREQ");
        break;
      }
      case aodv::AODVTYPE_RREP :
      {
        NS_LOG_LOGIC("Message found was an AODV::RREP");
        break;
      }
      case aodv::AODVTYPE_RERR :
      {
        NS_LOG_LOGIC("Message found was an AODV::RERR");
        break;
      }
      case aodv::AODVTYPE_RREP_ACK :
      {
        NS_LOG_LOGIC("Message found was an AODV::RREP_ACK");
        break;
      }
    }
  }
  return tHeader.IsValid();
}

void QLearner::ReceiveAodv(Ptr<Socket> socket) {
  // Currently UNUSED method designed to intercept AODV traffic to learn what neighbours are present.
  NS_FATAL_ERROR("currently not used.");
  NS_LOG_FUNCTION (this << socket);
  Address sourceAddress;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
  Ptr<Packet> packet_for_aodv = packet->Copy (); //Packet::Copy is meant for this ?

  aodvProto->RecvAodvWithoutRead (socket, packet_for_aodv, sourceAddress);

  aodv::TypeHeader tHeader (aodv::AODVTYPE_RREQ);
  packet->RemoveHeader (tHeader);
  if (!tHeader.IsValid ())
    {
      NS_LOG_DEBUG ("AODV message " << packet->GetUid () << " with unknown type received: " << tHeader.Get () << ". Drop");
      return; // drop
    }
  switch (tHeader.Get ())
  {
    case aodv::AODVTYPE_RREQ:
    case aodv::AODVTYPE_RREP_ACK:
    case aodv::AODVTYPE_RERR:
      {
        break;
      }
    case aodv::AODVTYPE_RREP:
    {
      aodv::RrepHeader rrepHeader;
      packet->RemoveHeader (rrepHeader);

      Ipv4Address dst = rrepHeader.GetDst ();
      /**
       *
       * if dst == rrepHeader.getOrigin (so a RREP from the dst, basically) and if receiver != that dst -> mark as neighb
       * and part is gratuitous, i dont think nodes parse the messages they themselves have sent, but to be sure...
       *
       */

      if (dst == rrepHeader.GetOrigin() && GetNode()->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal() != dst ) {
        // RegisterNeighbor(dst); <- removed
      }

      NS_LOG_DEBUG ("recv " << packet->GetSize () << " bytes");
      NS_ASSERT (InetSocketAddress::IsMatchingType (sourceAddress));
      break;
    }
  }

  /** Dont do this -- there is no ipv4 header to remove so dont try to remove it or you break everything
   * Ipv4Header ipv4;
   * packet->RemoveHeader (ipv4);
   */
}

void
QLearner::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}
//
// void PacketTracking (Ptr<const> Packet> p) {
//
// }

void QLearner::QLrnPacketTracking (Ptr<const Packet> p) {
  /**
   * Connect to trace that fires whenever a packet is sent by the QLearner, this allows us to count the number of
   * control packets being sent by each QLearning node seperately and also count the entire amount of packets being sent
   * in the simulation.
   */
  // Put this in a seperate function s.t. we dont have to add more logic for the other counting stuff ?
  m_control_packets_sent += 1;
  tmp.insert(p->GetUid());

  //Must be QLRN packets!
  QLrnHeader q;
  QoSQLrnHeader qosq;
  NS_ASSERT_MSG(CheckQLrnHeader_withoutUDP(p,q) || CheckQLrnHeader_withoutUDP(p,qosq), "this should have been a valid QLRN traffic since im the one who made / sent it ??");
}

void
QLearner::StartApplication (void)
{
  m_this_node_ip = GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
  TraceConnectWithoutContext ("Tx", MakeCallback(&QLearner::QLrnPacketTracking , this  ));

  std::stringstream ss;
  ss << "[node " << GetNode()->GetId () << "]    ";
  m_name = ss.str();

  NS_LOG_FUNCTION (this);

  Ptr<Ipv4L3Protocol> l3 = GetNode()->GetObject<Ipv4> ()->GetObject<Ipv4L3Protocol> ();
  if (m_socket == 0) {
      /* Grab the socket AODV created (note: this involved making a private fct public) */
      aodvProto = Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol>(GetNode()->GetObject<Ipv4> ()->GetRoutingProtocol ());
      aodvProto->SetQLearner(this);
      m_socket = aodvProto->FindSocketWithInterfaceAddress (GetNode()->GetObject<Ipv4> ()->GetAddress (1, 0));

      /* Create our own socket for QInfo traffic */

      m_qlrn_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
      m_qlrn_socket->Bind ( InetSocketAddress(l3->GetAddress (1,0).GetLocal(), QLRN_PORT) );
      m_qlrn_socket->SetRecvCallback( MakeCallback (&QLearner::Receive, this));
      m_qlrn_socket->BindToNetDevice (l3->GetNetDevice (1)); //1 but what if there is more than 1 (0 is localhost) ?
      m_qlrn_socket->SetAllowBroadcast (true);
      m_qlrn_socket->SetIpRecvTtl (true);

      /** Set a callback to call when an AODV packet is received
          - Does not interfere with AODV operation
          - Allows us to send an AODV HELLO message
          -   and to receive the replies, thus being able to check the neighbours that warranty

          Alternatively, we can make other fcts public in AODV and pull the Neighbours directly, but then you have to wait for AODV maybe?
          --> Right now we did the making public thing, the intercepting of AODV traffic has been disabled for now.

          Trying to accomplish this without making the function public leads only to socket collisions and incorrect addr:port being assigned to the socket...
          (this was verified)
       */
      //  m_socket->SetRecvCallback (MakeCallback (&QLearner::Recei veAodv, this));
  }
  /* connect to arp cache timeout thing */
  l3->GetInterface (1)->GetArpCache ()->TraceConnectWithoutContext ("MarkDead", MakeCallback(&QLearner::ARPDeadTrace, this ));

  Ptr<WifiNetDevice> wifiNetDevice;
  Ptr<AdhocWifiMac> adhocWifiMac;
  Ptr<WifiMacQueue > mac_queue;
  try {
    wifiNetDevice = dynamic_cast<WifiNetDevice*> ( PeekPointer(l3->GetNetDevice (1) ) );
    adhocWifiMac = dynamic_cast<AdhocWifiMac*> ( (PeekPointer(wifiNetDevice->GetMac()) ) );
    mac_queue = adhocWifiMac->GetTxop()->GetWifiMacQueue(); //GetDcaTxop()->GetQueue();
  } catch (std::exception & e) {
    NS_FATAL_ERROR("The MAC layer should be AdHoc, since that is the type of network we are working with.");
  }

  mac_queue->TraceConnectWithoutContext("EnqueuePacket",MakeCallback(&QLearner::MACEnqueuePacket, this));
  mac_queue->TraceConnectWithoutContext("DequeuePacket",MakeCallback(&QLearner::MACDequeuePacket, this));
  // MACEnqueuePacket
  // MACDequeuePacket

  /* Either do manually or use neighb from aodv*/
  bool AODV = true;

  if (AODV) {
    neighbours = FindNeighboursAODV ();
  } else {
    neighbours = FindNeighboursManual ();
  }

  m_qtable = QTable(neighbours, GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), m_learningrate,
                  m_qconvergence_threshold, m_learning_threshold, std::vector<Ipv4Address>(), "_web", m_in_test, m_print_qtables, m_gamma);
  m_qtable_video = QTable(neighbours, GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), m_learningrate,
                  m_qconvergence_threshold, m_learning_threshold, std::vector<Ipv4Address>(), "_video", m_in_test, m_print_qtables, m_gamma);
  m_qtable_voip = QTable(neighbours, GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), m_learningrate,
                  m_qconvergence_threshold, m_learning_threshold, std::vector<Ipv4Address>(), "_voip", m_in_test, m_print_qtables, m_gamma);

  if (GetNode()->GetId() == 27) {
    std::cout << m_qtable.PrettyPrint() << std::endl;
    std::cout << "\n======================BEGIN========================\n\n";
  }

  if (m_output_data_to_file && IamAmongTheDestinations() && m_output_filestream == 0) {
    std::stringstream ss; ss << "out_stats_q_node"<<GetNode()->GetId()<<".csv";
    m_output_filestream = Create<OutputStreamWrapper> (ss.str(), std::ios::out);
    *(m_output_filestream->GetStream ()) << "pktID,currTime,delay,initial_estim,learning,metric_delay,metric_jitter,metric_loss,second_to_last_hop,trafficType\n";
  }

  if (m_num_applications == 1 && real_traffic != 0 && m_use_learning_phases) {
    if (real_traffic/*->GetN() == 1*/ != 0) {
      try {
        dynamic_cast<OnOffApplication&> ( *(PeekPointer(real_traffic/*->Get(0)*/)) ).SetLearning(true);
      } catch (std::exception & bcf) {
        NS_FATAL_ERROR("We expect all traffic generators to be OnOffGenerators right now, due to the changes that were made to OnOffApplication to make it stop/start." << bcf.what());
      }
    }
  }
}

void QLearner::InitializeLearningPhases(std::vector<Ipv4Address> destinations) {
  for (const auto& i : destinations) {
    m_learning_phase[i] = true;
  }
}

void QLearner::MACEnqueuePacket( uint64_t pID, bool learning_traffic /*  ignore the packet if its learning tx */ ) {
  GetPacketTable()->EnqueuePacket(pID);
  // NS_LOG_UNCOND(m_name << "MAC LAYER IS ENQUEUEING PID " << pID << ".  Now check if AODV enqueued it.  "
  // << Simulator::Now().GetSeconds() << " " << GetPacketTable()->GetPacketQueueTime(pID,true)<< "mind, param true so its debugging");

}

void QLearner::MACDequeuePacket( uint64_t pID, bool learning_traffic /* ignore the packet if its learning tx */) {
  GetPacketTable()->DequeuePacket(pID);
  NS_ASSERT_MSG(GetPacketTable()->GetPacketQueueTime(pID) >= NanoSeconds(0), "Cant have a queue time < 0ns");
  // NS_LOG_UNCOND(m_name << "MAC LAYER IS DEQUEUEING PID " << pID << ".  Now check if AODV enqueued it.  "
  // << Simulator::Now().GetSeconds() << " " << GetPacketTable()->GetPacketQueueTime(pID,true)
  // << "mind, param true so its debugging");
}

void
QLearner::ARPDeadTrace( Ipv4Address downed_neighb )  {
  // std::cout << downed_neighb << " is down at " << m_this_node_ip << " " << Simulator::Now().As(Time::S)  <<"\n";
  NS_LOG_DEBUG("qlrn down   " << downed_neighb << " is down at " << m_this_node_ip << " " << Simulator::Now().As(Time::S) );
  // Down because not replying to arp's.
  NotifyLinkDown(downed_neighb);
}

void
QLearner::Receive(Ptr<Socket> socket) {
  Address sourceAddress;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
  Ipv4Address sourceIPAddress = InetSocketAddress::ConvertFrom (sourceAddress).GetIpv4 ();

  TrafficType t = OTHER;

  QLrnHeader qlrnHeader;
  packet->RemoveHeader (qlrnHeader);
  if (!qlrnHeader.IsValid ()) {
      NS_FATAL_ERROR("Incorrect QLrnHeader found."); //stop simulation
  }
  t = qlrnHeader.GetTrafficType();

  NS_LOG_DEBUG( m_name << "learning info about " << qlrnHeader.GetPDst() <<" from packet ID " << qlrnHeader.GetPktId()
            << " : travel time was " << Time::FromInteger(qlrnHeader.GetTime(), Time::NS).As(Time::MS) << " and next estim : " << Time::FromInteger(qlrnHeader.GetNextEstim(), Time::NS)
            << ". This info was contained in pkt " << packet->GetUid() << " sent by " <<  sourceIPAddress);
  GetQTable(t).Update(sourceIPAddress, //get the neighbour that we chose as next hop
                qlrnHeader.GetPDst(),   //the actual destination of the packet ( to know which entry in the QTable to update)
                m_packet_info.GetPacketQueueTime(qlrnHeader.GetPktId()) /*- Time::FromInteger(qlrnHeader.GetTime(), Time::NS)*/, //the time the packet spent in the queue
                Time::FromInteger(qlrnHeader.GetTime(), Time::NS), // //the time the packet spent in the air (so time between sent and arrival)
                Time::FromInteger(qlrnHeader.GetNextEstim(), Time::NS) //and also the next hop's estimate, as we need that
              );
  if (qlrnHeader.GetPDst() != sourceIPAddress) {
    NS_LOG_DEBUG( m_name << "[N] ALSO LEARNING abt " << sourceIPAddress <<" from packet ID " << qlrnHeader.GetPktId()
              << " : travel time was " << Time::FromInteger(qlrnHeader.GetTime(), Time::NS).As(Time::MS)
              << " GetPacketQueueTime time was " << m_packet_info.GetPacketQueueTime(qlrnHeader.GetPktId())
              << ". Contained in pkt " << packet->GetUid() << " sent by " <<  sourceIPAddress);
  GetQTable(t).Update(sourceIPAddress, //get the neighbour that we chose as next hop
                    sourceIPAddress,
          /*EXPERIMENTAL*/          m_packet_info.GetPacketQueueTime(qlrnHeader.GetPktId()), //the time the packet spent in the queue for nexthop neighbours, this should be 0 --
                    Time::FromInteger(qlrnHeader.GetTime(), Time::NS), // //the time the packet spent in the air (so time between sent and arrival)
                    Time::FromInteger(0, Time::NS) //and also the next hop's estimate, as we need that
                  );

  }

  GetQTable(TRAFFIC_A).GetEntryByRef(qlrnHeader.GetPDst(), sourceIPAddress).SetSenderConverged(qlrnHeader.GetSenderConverged());
  GetQTable(TRAFFIC_B).GetEntryByRef(qlrnHeader.GetPDst(), sourceIPAddress).SetSenderConverged(qlrnHeader.GetSenderConverged());
  GetQTable(TRAFFIC_C).GetEntryByRef(qlrnHeader.GetPDst(), sourceIPAddress).SetSenderConverged(qlrnHeader.GetSenderConverged());

  Ipv4Address destination = qlrnHeader.GetPDst() ;
  if (qlrnHeader.GetSenderConverged()) {
    //can do this instead of checking them one by one, exhaustively
    NS_ASSERT_MSG(std::find(m_traffic_destinations.begin(), m_traffic_destinations.end(), destination) != m_traffic_destinations.end() ||
                  (std::find(m_traffic_sources.begin(), m_traffic_sources.end(), destination) != m_traffic_sources.end() &&
                  (t == ICMP || t == UDP_ECHO )),"it shouldnt have been true but it is/was ... \n" << qlrnHeader << "\n"
                              << "traffictype = " << traffic_type_to_traffic_string(t) << " and the destination ip was " << destination);
  }
  // if (GetNode()->GetId()  == 1){
  //   NS_LOG_UNCOND("dst is " << destination << " and converged (best choice) for that is " << GetQTable(t).HasConverged(destination, true ) << "\n"
  //                   << m_num_applications << "  " << m_use_learning_phases << " " << m_learning_phase[destination] << "  " << m_my_sent_traffic_destination << "\n"
  //   );
  // }
  if (GetQTable(t).HasConverged(destination) && m_num_applications >= 1 && m_use_learning_phases && m_learning_phase[destination] && m_my_sent_traffic_destination == destination) {
    // NS_LOG_UNCOND(m_name << "(no qos) Im in learning phase, all choices have converged, time ( " << Simulator::Now().As(Time::S) << " ) to stop learning & start traffic. pkt id:" << packet->GetUid());
    StopLearningStartTraffic(destination);
    NS_ASSERT(GetNode()->GetId() == 0); // old assert for when node0 only has 1 next_hop
    // NS_FATAL_ERROR("i dont want this to happen?"); //-- but it happens anyway ?? the fk --- only happens in 0, because it only has 1 choice
  } else if (GetQTable(t).HasConverged(destination, true ) &&  m_num_applications >= 1  && m_use_learning_phases && m_learning_phase[destination] && m_my_sent_traffic_destination == destination) {
    // NS_LOG_UNCOND(m_name << "(no qos) Im in learning phase, best choice has converged, time ( " << Simulator::Now().As(Time::S) << " ) to stop learning & start traffic. pkt id:" << packet->GetUid());
    StopLearningStartTraffic(destination);
  } else if (GetQTable(t).HasConverged(destination, true ) && m_use_learning_phases && m_learning_phase[destination]) {
    SetLearningPhase(false,destination); //-- so the issue lies here, "best estim" is not necessarily really the best estim, and that ruins the simulation for 15 straight line nodes!
  } else if (GetQTable(t).HasConverged(destination) && m_use_learning_phases && m_learning_phase[destination]) {
    SetLearningPhase(false,destination); //Decide on this part still... best (i think) is to have only one  converge since thats all we need
  }


  // Because for one entyr it might be told to learn more, then another will send QInfo and the
  // decision may be to learn less. As these have different variables, they might cancel eachother out!
  // bool to_dst_learn_less = GetQTable(t).GetEntryByRef(qlrnHeader.GetPDst(),sourceIPAddress).LearnLess();
  // bool to_dst_learn_more = GetQTable(t).GetEntryByRef(qlrnHeader.GetPDst(),sourceIPAddress).LearnMore();
  bool to_dst_learn_less = GetQTable(t).LearnLess(qlrnHeader.GetPDst() );
  bool to_dst_learn_more = GetQTable(t).LearnMore(qlrnHeader.GetPDst() );
  NS_ASSERT_MSG(!(to_dst_learn_more && to_dst_learn_less) , "Cant have both at the same time, that would be conflicing ideas.");
  /*
   * dont increase or decrease learning for the neighbour, we care only about the dst!
   * values for neighb are much smaller so are %change more
   */
  bool to_neigh_learn_less = false; // GetQTable(t).GetEntryByRef(sourceIPAddress,sourceIPAddress).LearnMore();
  bool to_neigh_learn_more = false; // GetQTable(t).GetEntryByRef(sourceIPAddress,sourceIPAddress).LearnMore();

  if ( (to_dst_learn_more || to_neigh_learn_more ) && m_small_learning_stream && m_my_sent_traffic_destination == destination /* dont mess with traffic if its not our dst*/ &&
        m_use_learning_phases && !m_learning_phase[destination] && m_num_applications == 2 && learning_traffic_applications/*->GetN() == 1*/!=0 && real_traffic/*->GetN() == 1*/!=0) {
    // std::cout << (to_dst_learn_more ? "to dst learn more":"not to dst learn more" ) << std::endl;
    // std::cout << (to_neigh_learn_more ? "to neigh learn more":"not to neigh learn more" ) << std::endl;
    try {
      dynamic_cast<OnOffApplication&> ( *(PeekPointer(learning_traffic_applications/*->Get(0)*/)) ).IncreaseAmountOfTraffic(0,1.5); //--VANHIER
    } catch (std::bad_cast & e) {
      NS_FATAL_ERROR("bad cast was thrown & caught. learning_traffic_applications should be of the OnOffApplication variety though." << e.what() );
    }

  } else if ( ( to_dst_learn_less || to_neigh_learn_less ) && m_small_learning_stream && m_my_sent_traffic_destination == destination /* dont mess with traffic if its not our dst*/ &&
                m_use_learning_phases && !m_learning_phase[destination] && m_num_applications == 2 && learning_traffic_applications/*->GetN() == 1*/!=0 && real_traffic/*->GetN() == 1*/!=0) {
    try {
      dynamic_cast<OnOffApplication&> ( *(PeekPointer(learning_traffic_applications/*->Get(0)*/)) ).ReduceAmountOfTraffic(0,0.5); //--VANHIER
    } catch (std::bad_cast & e) {
      NS_FATAL_ERROR("bad cast was thrown & caught. learning_traffic_applications should be of the OnOffApplication variety though." << e.what() );
    }

  }
  return;
}


void
QLearner::StopTrafficStartLearning(Ipv4Address i) {
  NS_ASSERT_MSG(m_my_sent_traffic_destination == i, "Trying to stop traffic to a destination Im totally not sending traffic to!");
  // Stop the learning traffic thing
  NS_FATAL_ERROR("currently unused.");
  for (unsigned int i = 0; i < (learning_traffic_applications != 0 ? 1 : 0) ; i++) {
    try {
      if (!m_small_learning_stream) {
        dynamic_cast<OnOffApplication&> ( *(PeekPointer(learning_traffic_applications/*->Get(0)*/)) ).SetMaxBytes(0);
      } else {
        dynamic_cast<OnOffApplication&> ( *(PeekPointer(learning_traffic_applications/*->Get(0)*/)) ).IncreaseAmountOfTraffic();
      }
      dynamic_cast<OnOffApplication&> ( *(PeekPointer(real_traffic/*->Get(0)*/)) ).SetMaxBytes(FREEZE_ONOFFAPPLICATION_SENDING);
    } catch (std::exception & bc) {
      NS_FATAL_ERROR("We expect all traffic generators to be OnOffGenerators right now, due to the changes that were made to OnOffApplication to make it stop/start." << bc.what());
    }
  }
  SetLearningPhase(true, i);

  GetQTable(TRAFFIC_C).Unconverge();
  GetQTable(TRAFFIC_B).Unconverge();
  GetQTable(TRAFFIC_A).Unconverge();
}

void
QLearner::StopLearningStartTraffic(Ipv4Address i) {
  NS_ASSERT_MSG(m_my_sent_traffic_destination == i, "Trying to stop learning traffic to a destination Im totally not sending traffic to!  "
                  << m_my_sent_traffic_destination << " " << i);
  // std::cout << m_name << "  " << learning_traffic_applications/*->GetN()*/ << std::endl;
  for (unsigned int i = 0; i < (learning_traffic_applications != 0 ? 1 : 0) ; i++) {
    try {
      if (!m_small_learning_stream) {
        dynamic_cast<OnOffApplication&> ( *(PeekPointer(learning_traffic_applications/*->Get(0)*/)) ).SetMaxBytes(FREEZE_ONOFFAPPLICATION_SENDING);
      } else {
        dynamic_cast<OnOffApplication&> ( *(PeekPointer(learning_traffic_applications/*->Get(0)*/)) ).ReduceAmountOfTraffic();
      }
      dynamic_cast<OnOffApplication&> ( *(PeekPointer(real_traffic/*->Get(0)*/)) ).SetMaxBytes(0);
    } catch (std::exception & bc) {
      NS_FATAL_ERROR("We expect all traffic generators to be OnOffGenerators right now, due to the changes that were made to OnOffApplication to make it stop/start." << bc.what());
    }
  }
  SetLearningPhase(false,i);
  //workaround to let old tests work still
  if (m_num_applications == 1 && real_traffic != 0) {
    if (real_traffic/*->GetN() == 1*/!=0) {
      try {
        dynamic_cast<OnOffApplication&> ( *(PeekPointer(real_traffic/*->Get(0)*/)) ).SetLearning(false);
      } catch (std::exception & bc) {
        NS_FATAL_ERROR("We expect all traffic generators to be OnOffGenerators right now, due to the changes that were made to OnOffApplication to make it stop/start." << bc.what());
      }
    }
  }
}

void
QLearner::Send (Ipv4Address node_to_notify, uint64_t packet_Uid, Time travel_time, Ipv4Address packet_dst, Ipv4Address packet_src, TrafficType t, bool sender_converged)
{
  NS_ASSERT_MSG(node_to_notify != Ipv4Address(UNINITIALIZED_IP_ADDRESS_QINFO_TAG_VALUE), "supposed to be lrn traffic but the qlrn info tag is not initialised.");
  int delay = 0;
  if (m_jittery_qlearner) {
    NS_FATAL_ERROR("no longer used.");
    if (m_apply_jitter) {
      delay = m_delay * 1000000 ;
      m_apply_jitter = false;
    } else {
      m_apply_jitter = true;
    }
  }
  if (m_slow_qlearner) {
    NS_FATAL_ERROR("no longer used2.");
    delay = m_delay * 1000000;
  }

  QLrnHeader qLrnHeader ( packet_Uid, travel_time.GetInteger(), GetQTable(t).GetNextEstim(packet_dst).GetQValue().GetInteger()+delay, sender_converged, packet_dst, t);
  Ptr<Packet> packet = Create<Packet> ();
  // if (GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal() == Ipv4Address("10.1.1.8") ||
  //     GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal() == Ipv4Address("10.1.1.2")) {
  //   qLrnHeader = QLrnHeader( packet_Uid, travel_time.GetInteger(), GetQTable(t).GetNextEstim(packet_dst).second.GetInteger() * 100, packet_dst, t );
  // }
  packet->AddHeader (qLrnHeader);

  m_txTrace(packet);
  if (m_ideal) { // No real packet sending, only the learning part happens
	  NS_LOG_UNCOND("this is ideal");
	  std::cin.ignore();
    // Get a QTAgged packet -> go to the QLearner that is expecting some reply about this
    // He wants an update about the estimate for DST via ME and also an update about ME via ME

    // NS_LOG_DEBUG( m_name << "learning info about " << qlrnHeader.GetPDst() <<" from packet ID " << qlrnHeader.GetPktId()
    //           << " : travel time was " << Time::FromInteger(qlrnHeader.GetTime(), Time::NS).As(Time::MS) << " and next estim : " << Time::FromInteger(qlrnHeader.GetNextEstim(), Time::NS)
    //           << ". This info was contained in pkt " << packet->GetUid() << " sent by " <<  InetSocketAddress::ConvertFrom (sourceAddress).GetIpv4 ());
    // std::cout << Simulator::Now() << "  " << node_to_notify << std::endl;
    // std::cout << "Node" << GetNode()->GetId() << " " << packet_Uid << std::endl;

    m_other_qlearners[node_to_notify]->GetQTable(t).Update(GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), //We are the actual next hop
                    qLrnHeader.GetPDst(), //the actual destination of the packet ( to know which entry in the QTable to update)
                    m_packet_info.GetPacketQueueTime(qLrnHeader.GetPktId()), //the time the packet spent in the queue at me
                    Time::FromInteger(qLrnHeader.GetTime(), Time::NS) /*- m_packet_info.GetPacketQueueTime(qLrnHeader.GetPktId()) also old code */, //the time the packet spent in the air (so time between sent and arrival)
                    Time::FromInteger(qLrnHeader.GetNextEstim(), Time::NS) //and also the next hop's estimate, as we need that to update the value
                  );

    if (qLrnHeader.GetPDst() != GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal()) {
      // NS_LOG_DEBUG( m_name << "[N] ALSO LEARNING abt " << InetSocketAddress::ConvertFrom (sourceAddress).GetIpv4 () <<" from packet ID " << qlrnHeader.GetPktId()
      //           << " : travel time was " << Time::FromInteger(qlrnHeader.GetTime(), Time::NS).As(Time::MS)
      //           << " GetPacketQueueTime time was " << m_packet_info.GetPacketQueueTime(qlrnHeader.GetPktId())
      //           << ". Contained in pkt " << packet->GetUid() << " sent by " <<  InetSocketAddress::ConvertFrom (sourceAddress).GetIpv4 ());
      m_other_qlearners[node_to_notify]->GetQTable(t).Update(GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), //We are the actual next hop
                        GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(),
                        m_packet_info.GetPacketQueueTime(qLrnHeader.GetPktId()), //the time the packet spent in the queue for nexthop neighbours, this should be 0 -- [ if the queue is actually full .. but it should never be queued anyway]
                        Time::FromInteger(qLrnHeader.GetTime(), Time::NS) /*- m_packet_info.GetPacketQueueTime(qLrnHeader.GetPktId()) also old code */, // //the time the packet spent in the air (so time between sent and arrival)
                        Time::FromInteger(0, Time::NS) //and also the next hop's estimate, as we need that
                      );
    }

  } else {
    // Got a QInfoTagged packet -> reply with a QHeader packet
    m_qlrn_socket->SendTo (packet, 0, InetSocketAddress (node_to_notify, QLRN_PORT));
  }
}

void
QLearner::FixRoute (Ptr<Ipv4Route> route, Ptr<NetDevice> net, Ipv4Address src) {
  // route.SetDestination (Ipv4Address dest) already fine
  // route.SetGateway (Ipv4Address gw)
  route->SetOutputDevice (net);
  route->SetSource (src);
}

void
QLearner::NotifyLinkDown(Ipv4Address neighb) {
  m_qtable.MarkNeighbDown(neighb);
  m_qtable_voip.MarkNeighbDown(neighb);
  m_qtable_video.MarkNeighbDown(neighb);

  m_qtable.Unconverge();
  m_qtable_voip.Unconverge();
  m_qtable_video.Unconverge();

  for (const auto& dst : m_traffic_destinations ) {
    if (!m_learning_phase[dst] && !m_qtable.HasConverged(dst,true) ) {
      SetLearningPhase(true,dst);
    }
  }
}

void QLearner::AddNeighbour(Ipv4Address neighb) {
  m_qtable.AddNeighbour(neighb);
  m_qtable_voip.AddNeighbour(neighb);
  m_qtable_video.AddNeighbour(neighb);
}

void
QLearner::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }
  if (m_qlrn_socket != 0) {
    m_qlrn_socket->Close ();
    m_qlrn_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    m_qlrn_socket = 0;
  }
}

std::string
QLearner::GetStatistics() {
  std::stringstream ss;
  ss << m_control_packets_sent << "(" << tmp.size() << ")";
  return ss.str();
}

QTable& QLearner::GetQTable(TrafficType t) {
  if (t == ICMP || t == WEB || t == OTHER || t == UDP_ECHO || t == TRAFFIC_C) {
    return m_qtable;
  } else if (t == VIDEO|| t == TRAFFIC_B) {
    return m_qtable_video;
  } else if (t == VOIP || t == TRAFFIC_A) {
    return m_qtable_voip;
  } else {
    NS_FATAL_ERROR("unknown traffic.");
  }
}

bool QLearner::AddDestination (Ipv4Address via, Ipv4Address dst, Time t) {
  bool regular_table = m_qtable.AddDestination(via,dst,t);
  bool voip_table = m_qtable_voip.AddDestination(via,dst,t);
  bool video_table = m_qtable_video.AddDestination(via,dst,t);

  NS_ASSERT( regular_table == voip_table);
  NS_ASSERT( regular_table == video_table);
  return regular_table && voip_table && video_table;
}

bool QLearner::CheckDestinationKnown(const Ipv4Address& i) {
  NS_ASSERT(m_qtable.CheckDestinationKnown(i) == m_qtable_voip.CheckDestinationKnown(i));
  NS_ASSERT(m_qtable_voip.CheckDestinationKnown(i) == m_qtable_video.CheckDestinationKnown(i));
  return m_qtable.CheckDestinationKnown(i) && m_qtable_voip.CheckDestinationKnown(i) && m_qtable_video.CheckDestinationKnown(i);
}

Ipv4Address QLearner::GetNextHop(Ipv4Address dst, TrafficType t) {
  return GetQTable(t).GetNextEstim(dst).GetNextHop();
}

void QLearner::ChangeQValuesFromZero(Ipv4Address dst, Ipv4Address aodv_next_hop) {
  m_qtable.ChangeQValuesFromZero(dst, aodv_next_hop);
  m_qtable_voip.ChangeQValuesFromZero(dst, aodv_next_hop);
  m_qtable_video.ChangeQValuesFromZero(dst, aodv_next_hop);
}

std::string QLearner::PrintQTable(TrafficType t) {
  return GetQTable(t).PrettyPrint(traffic_type_to_traffic_string(t));
}

void QLearner::FinaliseQTables(TrafficType t) {
  GetQTable(t).FinalFile();
}

bool QLearner::SetOtherQLearners(NodeContainer nodes) {
  for (unsigned int i = 0; i < nodes.GetN(); i++) {
    for (unsigned int j = 0; j < nodes.Get(i)->GetNApplications(); j++) {
      try {
        Ptr<QLearner> qq = &dynamic_cast<QLearner&> (*(GetPointer(nodes.Get(i)->GetApplication (j)))); //this may become a problem!
        if (GetNode() != nodes.Get(i)){
          m_other_qlearners[nodes.Get(i)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal()] = qq;
          // std::cout << nodes.Get(i)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal() << " " << m_other_qlearners[nodes.Get(i)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal()] << std::endl;
        }
      } catch (const std::bad_cast &) {
        // do nothing, keep searching
      }
    }
  }
  // for node in nodes:
  //   other_qlearners insert <node.ip , ptr to node.qlrner>
  // this should only have size > 0 if there is ideal set , so then in the Receive -> Update function
  // OR IN SEND???
  // replace the send by using the ptr to do the update...
  return true;
}

void QLearner::RestartAfterMoving() {
  NS_LOG_DEBUG("RestartAfterMoving being called for node" << GetNode()->GetId() << " and my position is "
                  << GetNode()->GetObject<MobilityModel> ()->GetPosition().x
                  << "," << GetNode()->GetObject<MobilityModel> ()->GetPosition().y
                  << "," << GetNode()->GetObject<MobilityModel> ()->GetPosition().z );
  GetAODV()->SendHello();
}

void QLearner::UpdateAvgDelay(PacketTimeSentTag ptst_tag, PortNrTag pnt) {
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
  }
}

std::vector<std::pair< std::pair<Ipv4Address,Ipv4Address>, float > > QLearner::GetPacketLossPerNeighb() {
  std::vector<std::pair< std::pair<Ipv4Address,Ipv4Address>, float > > ret;

  for (auto x : m_packet_recv_stats_per_neighb) {
    ret.push_back(std::pair< std::pair<Ipv4Address,Ipv4Address>,float> (x.first, x.second.second));
  }

  return ret;
}

Time QLearner::TravelTimeHelper(QLrnInfoTag tag) {
  Time ret;

  if (tag.GetUsableDelay()) {
    ret = Simulator::Now() - tag.GetTime();
    m_backup_per_prev_hop_for_unusable_delay[tag.GetPrevHop()] = Simulator::Now() - tag.GetTime();
  } else {
    ret = m_backup_per_prev_hop_for_unusable_delay[tag.GetPrevHop()];
  }
  if (GetNode()->GetId() == 6 || GetNode()->GetId() == 13) {
    // std::cout << " At node " << m_name << " obs delay has a value of " << ret.GetSeconds() << std::endl;
  }


  return ret;
}

void QLearner::OutputDataToFile(PacketTimeSentTag ptst_tag, Ptr<const Packet> p, bool learning_packet, TrafficType t,Ipv4Address sourceIP) {
  PortNrTag pnt;
  NS_ASSERT(p->PeekPacketTag(pnt));
  uint64_t currDelay = (Simulator::Now() - ptst_tag.GetSentTime()).GetInteger();

  m_prev_delay = (m_prev_delay_per_prev_hop[ptst_tag.GetPrevHop()] == 0? currDelay:m_prev_delay_per_prev_hop[ptst_tag.GetPrevHop()]);
  m_prev_delay_per_prev_hop[ptst_tag.GetPrevHop()] = currDelay;

  uint64_t jitter_val = ( currDelay < m_prev_delay ? m_prev_delay - currDelay : currDelay - m_prev_delay );

  /* if packet loss suddenly no longer works, observe this : TODO TODO TODO ?*/
  float total_traffic_recv_by_me = float(GetAODV()->RecvTrafficStatistics(sourceIP) + (pnt.GetLearningPkt() ? 1 : 0));
  float total_traffic_sent_to_me = float(pnt.GetPktNumber() + (!pnt.GetLearningPkt() ? 1 : 0)) /*because 1/0 is undefined, we add one (pkt number is 0-index)*/ ;

  float packet_loss = 1 - total_traffic_recv_by_me / (pnt.GetLearningPkt() ? total_traffic_recv_by_me : total_traffic_sent_to_me);
  // std::cout << total_traffic_recv_by_me << "  " << total_traffic_sent_to_me << std::endl;

  if (packet_loss < 0) {
    // std::cout << m_name << " got " << total_traffic_recv_by_me << " from " << sourceIP  << "  adn thinks it should be " << total_traffic_sent_to_me << std::endl;
    // std::cout << GetAODV()->RecvTrafficStatistics() << std::endl;
    NS_ASSERT(m_other_qlearners[sourceIP]->AllNeighboursBlacklisted(m_this_node_ip,t));
    /* because multiple paths are being used in the case of all neighb blacklisted, and each paht differnet delay */
    if (packet_loss > -0.01 || (total_traffic_recv_by_me < 100 && total_traffic_sent_to_me < 100 )) {
      /* fine, allow it.. */
      packet_loss = 1.0;
    } else {
      packet_loss = 0;
    }
  }

  bool delay_ok = currDelay < TrafficTypeReqsMap[t].GetDelayMax();
  bool jitter_ok = jitter_val < TrafficTypeReqsMap[t].GetJitterMax();
  // std::cout << packet_loss << std::endl;
  bool packet_loss_ok = packet_loss < TrafficTypeReqsMap[t].GetRandomLossMax();

  NS_ASSERT(packet_loss >= 0);
  // if (packet_loss < 0) { packet_loss = 0; }

  *(m_output_filestream->GetStream()) << p->GetUid() << "," << Simulator::Now().As(Time::S) << ","
          << (Simulator::Now() - ptst_tag.GetSentTime()).As(Time::MS) << "," << ptst_tag.GetInitialEstim();
  if (learning_packet)  { *(m_output_filestream->GetStream()) << ",learning,OK,OK,OK"; }
  else                  {
    *(m_output_filestream->GetStream()) << ",not_learning"
        << (delay_ok?",OK":",NOK")
        << (jitter_ok?",OK":",NOK")
        << (packet_loss_ok?",OK":",NOK");
  }
  *(m_output_filestream->GetStream()) << "," << ptst_tag.GetPrevHop();
  *(m_output_filestream->GetStream()) << "," << traffic_type_to_traffic_string(t) << "\n";

  (m_output_filestream->GetStream())->flush();
}

void QLearner::SetLearningTrafficGen( Ptr<Application> a ) {
  m_num_applications += 1;
  learning_traffic_applications = a;
}

void QLearner::SetTrafficGen(  Ptr<Application> a ) {
  m_num_applications += 1;
  real_traffic = a;
}

} // Namespace ns3
