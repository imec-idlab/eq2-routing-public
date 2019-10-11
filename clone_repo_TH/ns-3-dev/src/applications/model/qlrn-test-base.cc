#include "qlrn-test-base.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QLearningBase");

QLearningBase::QLearningBase () :
  numberOfNodes (10),
  totalTime (60),
  pcap (false),
  printRoutes (false),
  printQTables(false),
  linkBreak (false),
  linkUnBreak (false),
  qlearn(true),
  numHops(4),
  node_to_break(999999),
  eps(0.05),
  learning_rate(0.5),
  fixLocs(false),
  traffic("ping"),
  traffic_x("NOTHING"),
  ideal(false),
  in_test(false),
  interference_node(-1),
  learning_phases(false),
  rho(Q_INTERMEDIATE_OPTIMAL_RHO),
  max_retry(MAX_RETRY_DEFAULT),
  q_conv_thresh(QTABLE_CONVERGENCE_THRESHOLD),
  learn_more_threshold(LEARNING_PHASE_START_THRESHOLD),
  m_type_of_run("n/a"),
  metrics_back_to_src(false),
  smaller_learning_traffic(false),
  interferer(0)
{
  DataGeneratingApplications = ApplicationContainer();
  LearningGeneratingApplications = ApplicationContainer();
  DataSinkApplications = ApplicationContainer();
  // EventDataGeneratingApplications = std::vector<ApplicationContainer>();
  // EventLearningGeneratingApplications = std::vector<ApplicationContainer>();
  // EventDataSinkApplications = std::vector<ApplicationContainer>();
  EventDataGeneratingApplications =  ApplicationContainer();
  EventLearningGeneratingApplications =  ApplicationContainer();
  EventDataSinkApplications =  ApplicationContainer();

  mobility = MobilityHelper();
  aodv = AodvHelper();

  traffic_sources = std::vector<Ipv4Address>();
  traffic_destinations = std::vector<Ipv4Address>();
}

bool QLearningBase::ConfigureTest(
  bool _pcap,
  bool _printRoutes ,
  bool _linkBreak,
  bool _linkUnBreak,
  bool _ideal,
  unsigned int _numHops,
  std::string _test_case_filename,
  std::string _test_result_filename
  )
{
  in_test = true;

  pcap = _pcap;
  printRoutes = _printRoutes;
  linkBreak = _linkBreak;
  linkUnBreak = _linkUnBreak;
  numHops = _numHops;
  test_case_filename = _test_case_filename;
  ideal = _ideal;
  try {
    if (!test_case.FromFile("QLearningTests/" + test_case_filename)) { NS_FATAL_ERROR("loading test_case failed.");}
    numberOfNodes = test_case.GetNoN();
    totalTime = test_case.GetTotalTime();
    traffic = test_case.GetTrafficType();
    eps = test_case.GetEps();
    gamma = test_case.GetGamma();
    learning_rate = test_case.GetLearningRate();
    ideal = test_case.GetIdeal();
    m_qos_qlearning = test_case.GetQoSQLearning();
    m_output_stats = false;
    enable_flow_monitor = false;
    max_packets = test_case.GetMaxPackets();
    metrics_back_to_src = test_case.GetMetricsBackToSrcEnabled();
    smaller_learning_traffic = test_case.GetSmallLearningTraffic();
    max_retry = test_case.GetMaxRetry();
    rho = test_case.GetRho();
    q_conv_thresh = test_case.GetQConvThresh();
    learn_more_threshold = test_case.GetLrnMoreThreshold();

    for (const auto& i : test_case.GetBreakIds()) {
      NS_ASSERT(i > 0 && i < int(numberOfNodes)); //just to see if its possible to do this iteration in the try block
    }
    for (const auto& i : test_case.GetEvents()) {
      NS_ASSERT(i.node_id >= 0 && i.node_id < int(numberOfNodes)); //just to see if its possible to do this iteration in the try block
    }
    qlearn = test_case.GetQLearningEnabled();
    learning_phases = test_case.GetLearningPhases();
    if (m_type_of_run == "aodv") {
      std::cout << "=== Doing an AODV simulation... ===" << std::endl;
      qlearn = false;
      m_qos_qlearning = false;
      m_output_stats = true;
    } else if (m_type_of_run == "q" ) {
      std::cout << "=== Doing an Q simulation... ===" << std::endl;
      qlearn = true;
      m_qos_qlearning = false;
      m_output_stats = true;
    } else if (m_type_of_run == "qosq" ) {
      std::cout << "=== Doing an QoSQ simulation... ===" << std::endl;
      qlearn = true;
      m_qos_qlearning = true;
      m_output_stats = true;
      if (traffic_x != "NOTHING") {
        traffic = traffic_x;
      }
      // do also the traffic param ... ?
    } else {
      NS_ASSERT(m_type_of_run == "n/a");
    }

  } catch (std::exception & e ) {
    return false;
  }

  if (m_type_of_run == "n/a") {
    try {
      if (!test_results.FromFile("QLearningTests/" + _test_result_filename)) { NS_FATAL_ERROR("loading test_case failed.");}
    } catch (std::exception & e ) {
      return false;
    }
  } else {
    printQTables = true;
  }

  if (test_case.GetNoN() != test_results.GetNoN() && test_results.IsLoaded() ) {
    std::cout << test_case.GetNoN() << "  " << test_results.GetNoN() << " \n";
    std::cout << "missing result for every node." << std::endl;
    return false;
  }

  if (numberOfNodes <= 0 || totalTime <= 0) {
    std::cout << "numberOfNodes 0 or totalTime 0" << std::endl;
    return false;
  }
  if (traffic != "ping" && traffic != "web" && traffic != "video"
      && traffic != "voip" && traffic != "udp-echo"
      && traffic!="udp-but-no-echo-just-for-maxPkt"
      && traffic!="trafficA" && traffic!="trafficB" && traffic!="trafficC" ) {
    if (traffic.find('/') != std::string::npos) {
      std::stringstream test(traffic);
      std::string segment;
      while(std::getline(test, segment, '/')) {
        if (segment != "ping" && segment != "web" && segment != "video"
            && segment != "voip" && segment != "udp-echo"
            && segment!="udp-but-no-echo-just-for-maxPkt"
            && segment!="trafficA" && segment!="trafficB" && segment!="trafficC" ) {
          std::cout << "traffictype issue (>1 specified)" << std::endl;
          return false;
        }
      }
    } else {
      std::cout << "traffictype issue" << std::endl;
      return false;
    }
  }
  return true;
}

bool
QLearningBase::Configure (int argc, char **argv)
{
  //Set Random seed
  ns3::RngSeedManager::SetSeed(1338);

  CommandLine cmd;

  bool tmp_no_q = false;
  m_qos_qlearning = false;
  enable_flow_monitor = false;
  m_output_stats = false;

  cmd.AddValue ("pcap", "enable / disable pcap trace output", pcap);
  cmd.AddValue ("printRoutes", "enable / disable routing table output", printRoutes);
  cmd.AddValue ("printQTables", "enable / disable printing of QTables", printQTables);
  cmd.AddValue ("numberOfNodes", "Number of nodes in the net, larger than 1", numberOfNodes);
  cmd.AddValue ("totalTime", "Simulation time in seconds", totalTime);
  cmd.AddValue ("linkBreak", "Makes some node part of the path between src and dst unresponsive.", linkBreak);
  cmd.AddValue ("linkUnBreak", "Makes the node that was at one point made unresponsive, responsive again.", linkUnBreak);
  cmd.AddValue ("numHops", "Number of hops that we should travel downstream to find a node to sever connection to.", numHops);
  cmd.AddValue ("fixLocs", "Fix the locations of the various nodes for easier testing / debugging.", fixLocs);
  cmd.AddValue ("doTest", "Use a testfile to create a configuration of nodes instead of randomly / manually setting them. Please specify a filename in the QLearningTests folder.", test_case_filename);
  cmd.AddValue ("no_q", "If this option is set, QLearning will be disabled.", tmp_no_q);
  cmd.AddValue ("eps", "Specify the epsilon value to use in the e-greedy strategy of QLearner", eps);
  cmd.AddValue ("learn", "Specify the learning rate to use in the QLearner", learning_rate);
  cmd.AddValue ("traffic", "Choose the type of traffic you want to generate (VoIP (=voip), Video (=video), Web (=web), ICMP (=ping), generic UDP (=udp) )", traffic);
  cmd.AddValue ("ideal", "Ideal : no transmitting of QLRN packets, just directly doing it via pointers. \"cheating\"." ,ideal);
  cmd.AddValue ("unit_test_situation", "unit_test_situation", in_test);
  cmd.AddValue ("interference_node", "interference_node", interference_node);
  cmd.AddValue ("learning_phases", "use or dont use learning phases / traffic phases", learning_phases);
  cmd.AddValue ("qos_qlearning", "use or dont use qos when qlearning", m_qos_qlearning);
  cmd.AddValue ("maxpackets", "maximum packets to send from src (if relevant", max_packets);
  cmd.AddValue ("use_flowmon", "use flowmon or not", enable_flow_monitor);
  cmd.AddValue ("output_stats", "output stats (for now, delay of packets) to a file from dst qlearner", m_output_stats);
  cmd.AddValue ("metrics_back_to_src", "send metrics about packets back from dst to src (via ptr)", metrics_back_to_src);
  cmd.AddValue ("smaller_learning_traffic", "leave a small stream of learning data during data phase", smaller_learning_traffic);
  cmd.AddValue ("q_conv_thresh", "threshold in % for allowable difference in QValues to be called converged.", q_conv_thresh);
  cmd.AddValue ("rho", "optimal decision for learning tfx in intermediate nodes", rho);
  cmd.AddValue ("max_retry", "max times to retransmit a traffic packet before attaching a learning header to hopefully be able to learn more to solve it", max_retry);
  cmd.AddValue ("learn_more_threshold", "for when qvalues differ this much in percent we must learn more", learn_more_threshold);
  cmd.AddValue ("a", "use different kinds of algorithms", m_type_of_run);
  cmd.AddValue ("traffic_miguel_test", "set diff kind of traffic for miguel tests", traffic_x);

  cmd.Parse (argc, argv);

  if (in_test) {
    NS_ASSERT(test_case_filename != "");
    return ConfigureTest (    pcap /* pcap */, false /*printRoutes*/,
                              false /*linkBreak*/, false /* linkUnBreak*/,
                              false /*ideal*/, 0 /* numHops */,
                              test_case_filename/* test_case_filename */, test_case_filename.substr(0,test_case_filename.size()-4) + "_expected_results.txt");
  }


  if (test_case_filename != "") {
    if (!test_case.FromFile("QLearningTests/" + test_case_filename)) { NS_FATAL_ERROR("loading test_case failed. Check the input file."); }
    numberOfNodes = test_case.GetNoN();
  } else {
    // Do nothing
  }
  NS_ASSERT_MSG (numHops <= numberOfNodes or not linkBreak, "We wont be able to trigger a link breaking this far downstream as the path cannot possibly be that long.");
  NS_ASSERT_MSG (numberOfNodes > 1, "We need at least 2 nodes, otherwise there is no traffic to analyse.");

  if (tmp_no_q) {
    qlearn = false;
  }

  NS_ASSERT_MSG((!m_qos_qlearning && !qlearn) || (qlearn && m_qos_qlearning) || (qlearn && !m_qos_qlearning),\
    "Cant have qos qlearning enabled and not qlearn enabled.");
  return true;
}


void
QLearningBase::ReportStr( StringValue str) {
  std::cout << str.Get() << std::endl;
}

void QLearningBase::UnBreakNode() {
  Ptr<MobilityModel> mob = nodes.Get (node_to_break)->GetObject<MobilityModel> ();
  Ptr<QLearner> qq = dynamic_cast<QLearner*> (   PeekPointer( QLearners.Get (node_to_break) ) );
  auto x = test_case.GetLocationForNode(node_to_break);
  PrintLocationsToFile("locations_broken.txt", false);
  Simulator::Schedule (Seconds (1), &MobilityModel::SetPosition, mob, Vector (x.first, x.second, 0  ));
  Simulator::Schedule (Seconds (1), &QLearner::RestartAfterMoving, qq );
  Simulator::Schedule (Seconds (2), &QLearningBase::PrintLocationsToFile, this, "locations_restored.txt", true);

  // mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
  //   "MinX", DoubleValue (location.first),
  //   "MinY", DoubleValue (location.second));
  // mobility.Install (nodes.Get (index++));
  //
  // for (const auto& location : test_case.GetLocations()) {
  // Simulator::Schedule (Seconds (1), &QLearningBase::ReportStr, this, StringValue(oss.str()));

}

void
QLearningBase::DetermineNodeToBreak () {
  aodv::RoutingTableEntry toDst;

  Ptr<Ipv4RoutingProtocol> rProto = nodes.Get(0)->GetObject<Ipv4> ()->GetRoutingProtocol ();

  /* use this method... */
  Ptr<aodv::RoutingProtocol> aProto = Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol> (rProto);

  /* instead of this one. This also works, but clearly the GetRouting approach is preferable */
  // aodv::RoutingProtocol bProto = dynamic_cast<aodv::RoutingProtocol&> (*rProto);
  // aProto->GetRoutingTable ().LookupRoute (interfaces.GetAddress (numberOfNodes-1), toDst);
  // bProto.GetRoutingTable ().LookupRoute (interfaces.GetAddress (numberOfNodes-1), toDst);
  unsigned int i = 0;
  int next_hop = -1;
  while (i < numHops) {
    aProto->GetRoutingTable ().LookupRoute (interfaces.GetAddress (numberOfNodes-1), toDst);
    for (unsigned int j = 1; j < numberOfNodes; j++) {
      if (interfaces.GetAddress(j) == toDst.GetNextHop ()) {
        next_hop = j;
      }
    }

    NS_ASSERT_MSG (toDst.GetNextHop() != Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE), "No route to the destination node for ICMP traffic could be found!");
    NS_ASSERT (next_hop > 0);

    rProto = nodes.Get(next_hop)->GetObject<Ipv4> ()->GetRoutingProtocol ();
    aProto = Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol> (rProto);
    i += 1;
  }
  node_to_break = next_hop;

  std::ostringstream oss;
  Ptr<MobilityModel> mob = nodes.Get (node_to_break)->GetObject<MobilityModel> ();

  oss << "\n\nBreaking node " << node_to_break << " ( " << numHops << " hops downstream from src), at time t = 920\n\n";

  Simulator::Schedule (Seconds (1), &QLearningBase::ReportStr, this, StringValue(oss.str()));
  Simulator::Schedule (Seconds (1), &MobilityModel::SetPosition, mob, Vector (1e5, 1e5, 1e5));
  if (linkUnBreak) {
    Simulator::Schedule (Seconds (25),  &QLearningBase::UnBreakNode, this);
  }
}

void
QLearningBase::ReportProgress() {
  std::cout << "Currently at time T = " << Simulator::Now().As(Time::S) << ".\n";
  Simulator::Schedule (Seconds (99), &QLearningBase::ReportProgress, this);
}
void QLearningBase::HandleTrafficEvent(TestEvent i) {
  NS_ASSERT_MSG(i.et == TRAFFIC, "obviously it should be a traffic event for this fct");
  NS_ASSERT_MSG(i.t > 1, "Otherwise will get a nasty error because of time going below 0.");
  // setup a traffic stream thing
  // set its start & stop according to some params
  // dont need to schedule anythign else
  // ApplicationContainer send;
  // ApplicationContainer sink;
  // ApplicationContainer lrn;

  if (qlearn) {
    InstallEventTraffic(EventDataGeneratingApplications, EventDataSinkApplications, 300, i.new_value, traffic_string_to_port_number(i.traffic_type)+10,
                        i.node_id, i.node_rx, false);
    InstallVoIPTraffic(EventLearningGeneratingApplications, EventDataSinkApplications,traffic_string_to_port_number(i.traffic_type)+10,
                              true/*learning*/, i.node_id, i.node_rx);
  } else {
    InstallEventTraffic(EventDataGeneratingApplications, EventDataSinkApplications, 300, i.new_value, traffic_string_to_port_number(i.traffic_type)+10,
                        i.node_id, i.node_rx, false);
  }
  ApplicationContainer tmp_lrn;
  if (EventLearningGeneratingApplications.GetN() > 0) {
    tmp_lrn = ApplicationContainer(EventLearningGeneratingApplications.Get(EventLearningGeneratingApplications.GetN()-1));
  }
  ApplicationContainer tmp_data = ApplicationContainer(EventDataGeneratingApplications.Get(EventDataGeneratingApplications.GetN()-1));
  ApplicationContainer tmp_sink = ApplicationContainer(EventDataSinkApplications.Get(EventDataGeneratingApplications.GetN()-1));
  tmp_data.Start(Seconds(i.t));
  tmp_data.Stop(Seconds(i.t + i.seconds));
  tmp_lrn.Start(Seconds(i.t));
  tmp_lrn.Stop(Seconds(i.t + i.seconds));
  tmp_sink.Start(Seconds(i.t-1));
  tmp_sink.Stop(Seconds(i.t + i.seconds+1));

  // EventDataSinkApplications.push_back(sink);
  // EventDataGeneratingApplications.push_back(send);
  // EventLearningGeneratingApplications.push_back(lrn);

  // std::cout << EventDataSinkApplications.size() << "  " << EventDataGeneratingApplications.size() << "  "  << EventLearningGeneratingApplications.size() << std::endl;

  // if (dynamic_cast<QLearner*> ( PeekPointer( QLearners.Get (i.node_id) ) )->IsInLearningPhase(interfaces.GetAddress(i.node_rx))) {
  //   std::cout << " its in learning phase..\n"; }else{ std::cout<<" it is NOT in learning phase" << std::endl;
  // }

  // dynamic_cast<QLearner*> ( PeekPointer( QLearners.Get (i.node_id) ) )->SetLearningTrafficGen(EventLearningGeneratingApplications.at(EventLearningGeneratingApplications.size()-1));
  // dynamic_cast<QLearner*> ( PeekPointer( QLearners.Get (i.node_id) ) )->SetTrafficGen(EventDataGeneratingApplications.at(EventDataGeneratingApplications.size()-1));
  // dynamic_cast<OnOffApplication&> ( *(PeekPointer(EventLearningGeneratingApplications.at(EventLearningGeneratingApplications.size()-1).Get(0) ) ) ).SetMaxBytes(FREEZE_ONOFFAPPLICATION_SENDING);
  // dynamic_cast<OnOffApplication&> ( *(PeekPointer(EventDataGeneratingApplications.at(EventDataGeneratingApplications.size()-1).Get(0) ) ) ).SetMaxBytes(FREEZE_ONOFFAPPLICATION_SENDING);

  if (qlearn) {
    auto qq = dynamic_cast<QLearner*> ( PeekPointer( QLearners.Get (i.node_id) ) );
    qq->SetLearningTrafficGen(
      EventLearningGeneratingApplications.Get(EventLearningGeneratingApplications.GetN()-1)
    );
    qq->SetTrafficGen(
      EventDataGeneratingApplications.Get(EventDataGeneratingApplications.GetN()-1)
    );
    Simulator::Schedule (Seconds (i.t), &QLearner::SetLearningPhase, qq, true, interfaces.GetAddress(i.node_rx));

    dynamic_cast<OnOffApplication&> ( *(PeekPointer(EventLearningGeneratingApplications.Get(EventLearningGeneratingApplications.GetN()-1) ) ) ).SetMaxBytes(FREEZE_ONOFFAPPLICATION_SENDING);
    dynamic_cast<OnOffApplication&> ( *(PeekPointer(EventDataGeneratingApplications.Get(EventDataGeneratingApplications.GetN()-1) ) ) ).SetMaxBytes(FREEZE_ONOFFAPPLICATION_SENDING);
  } else {
    // dont freeze either onoff appl
  }


  if (qlearn && tmp_lrn.GetN() > 0) {
    dynamic_cast<QLearner*> ( PeekPointer( QLearners.Get (i.node_id) ) )->SetSmallLrnTraffic(smaller_learning_traffic);
  }
  traffic_sources.push_back(interfaces.GetAddress (i.node_id));
  traffic_destinations.push_back(interfaces.GetAddress (i.node_rx));

  // if (!qlearn) {
  //   Ptr<aodv::RoutingProtocol> aodvProto_src = Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol>(nodes.Get(i.node_id)->GetObject<Ipv4> ()->GetRoutingProtocol ());
  //   Ptr<aodv::RoutingProtocol> aodvProto_dst = Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol>(nodes.Get(i.node_rx)->GetObject<Ipv4> ()->GetRoutingProtocol ());
  //   aodvProto_dst->SetSrcAODV(aodvProto_src, interfaces.GetAddress(i.node_id));
  // }
  }

int
QLearningBase::Run ()
{
  if (in_test) {
    std::cerr << StringTestInfo();
    std::string err="";
    if (RunTest (err)) {
      std::cerr << "[TEST PASSED]" << err << std::endl;
    } else {
      std::cerr << "[TEST FAILED] --- " << err << std::endl;
      return 1; //Error
    }
    std::cerr << "==========================================================================================\n";
    return 0;
  }
  CreateAndPlaceNodes ();
  CreateDevices ();
  InstallInternetStack ();
  InstallApplications ();

  if (interference_node != -1) {
    // Simulator::Schedule (Seconds (4.1), &QLearningBase::SetupAODVToDst, this);
    // SetupAODVToDst();
    CreateAnInterferer(interference_node);
  }

  if (!in_test) {
    std::cout << "Starting simulation for " << totalTime << " s ...\n";
  }

  if (qlearn && printRoutes) {
    Simulator::Schedule (Seconds (25), &QLearningBase::PrintQRouteToFile, this, "qroute.txt", interfaces.GetAddress(0),interfaces.GetAddress(numberOfNodes-1));
    Simulator::Schedule (Seconds (totalTime-10), &QLearningBase::PrintQRouteToFile, this, "qroute.txt", interfaces.GetAddress(0),interfaces.GetAddress(numberOfNodes-1));
  }

  Simulator::Stop (Seconds (totalTime));
  Simulator::Schedule (Seconds (99), &QLearningBase::ReportProgress, this);

  InterconnectQLearners();
  if (enable_flow_monitor) { flowmon = flowmonHelper.InstallAll(); }

  for (const auto& i : test_case.GetEvents()) {
    if (i.et == TRAFFIC) {
      HandleTrafficEvent(i);
      dynamic_cast<QLearner*> ( PeekPointer( QLearners.Get (i.node_id) ) )->SetMyTrafficDst(interfaces.GetAddress(i.node_rx));
    } else {
      Simulator::Schedule (Seconds (i.t), &QLearningBase::ExecuteEvent, this, i);
    }
  }

  //Because only here do we know all the traffic destinations
  if (qlearn) {
    for (unsigned int j = 0; j < numberOfNodes; j++) {
      auto qq = dynamic_cast<QLearner*> ( PeekPointer( QLearners.Get (j) ) );
      qq->InitializeLearningPhases(traffic_destinations);
      if (!traffic.compare("ping") || !traffic.compare("udp-echo")) { qq->InitializeLearningPhases(traffic_sources); }
      qq->SetTrafficSources(traffic_sources);
      qq->SetTrafficDestinations(traffic_destinations);
    }
  } else {
    for (unsigned int i = 0; i < numberOfNodes; i++) {
      Ptr<aodv::RoutingProtocol> aodvProto = Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol>(nodes.Get(i)->GetObject<Ipv4> ()->GetRoutingProtocol ());
      aodvProto->SetTrafficDestinations(traffic_destinations);
      aodvProto->SetAttribute("OutputData", BooleanValue( std::find(traffic_destinations.begin(), traffic_destinations.end(), interfaces.GetAddress(i) ) != traffic_destinations.end() ) );
    }
  }

  Simulator::Run ();


  // For checking icmp dropped-ness
  // auto pp = dynamic_cast<V4Ping&>  (*DataGeneratingApplications.Get(0)) ;
  // std::cout << pp.m_sent.size() << " " <<  3 << std::endl;
  // std::cout << pp.m_recv << " " <<  3 << std::endl;

  // flowmon printing, taken from lab-2-solved.cc
  // Print per flow statistics
  if (enable_flow_monitor) {
    flowmon->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats ();

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter) {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);
      if ((t.sourceAddress == interfaces.GetAddress(0) && t.destinationAddress == interfaces.GetAddress(numberOfNodes-1))){
        NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
        NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
        NS_LOG_UNCOND("Rx Packets = " << iter->second.rxPackets);
        NS_LOG_UNCOND("Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps");
      }
    }
    flowmon->SerializeToXmlFile("thomasaodv_output.xml", true, true);
  }


  std::stringstream Astats, Qstats,Tstats,TLRNstats,TRECstats,TLRNRECstats;
  std::map<Ipv4Address,Time> avg_delay;
  std::map<Ipv4Address,std::vector<std::pair<std::pair<Ipv4Address,Ipv4Address>,float> > > packet_loss_per_neighb_from_src;
  Qstats << "node0(src)/";
  for (unsigned int i = 1; i < numberOfNodes-1; i++) {
    Qstats << "node" << i << "/";
  }
  Qstats << "node" << numberOfNodes-1 <<"(dst)/" << std::endl;
  if (qlearn) {
    for (unsigned int i = 0; i < numberOfNodes; i++) {
      auto qq = dynamic_cast<QLearner*> (   PeekPointer( QLearners.Get(i) ) );
      // auto qq = dynamic_cast<QLearner&> (*(nodes.Get(i)->GetApplication (j)));
      if (i < 7  && !in_test) {
        // std::cout << qq.PrintQTable(traffic_string_to_traffic_type(traffic)) << std::endl; // printing of q table info to stdout

        // CHecking aodv's neighbours . . .
        // for (auto const& x : qq.GetAODV()->GetNeighbors()->GetVector()) {
          // std::cout << x.m_neighborAddress << std::endl;
          // neighbours_fromGetVector.push_back(x.m_neighborAddress);
        // }
      }
      if (std::find(traffic_destinations.begin(), traffic_destinations.end(), interfaces.GetAddress(i)) != traffic_destinations.end()) {
      // if (i == numberOfNodes-1) {
        avg_delay[interfaces.GetAddress(i)] = Time::FromInteger(int(qq->AvgDelayAsFloat()), Time::NS);
      }
      if (std::find(traffic_sources.begin(), traffic_sources.end(), interfaces.GetAddress(i)) != traffic_sources.end()) {
      // else if (i == 0) {
        // std::cout << " -------------- " << interfaces.GetAddress(i) << std::endl;
        packet_loss_per_neighb_from_src[interfaces.GetAddress(i)] = qq->GetPacketLossPerNeighb();
      }
      qq->FinaliseQTables(traffic_string_to_traffic_type("trafficA"));
      qq->FinaliseQTables(traffic_string_to_traffic_type("trafficB"));
      qq->FinaliseQTables(traffic_string_to_traffic_type("trafficC"));
      Qstats << qq->GetStatistics() << '/';
    }
  }
  Qstats << std::endl << "QLRN TRAFFIC:   ";
  for (unsigned int i = 0; i < numberOfNodes; i++) {
    Ptr<aodv::RoutingProtocol> aodvProto = Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol>(nodes.Get(i)->GetObject<Ipv4> ()->GetRoutingProtocol ());
    Qstats << aodvProto->GetQStatistics() << '/';
    Astats << aodvProto->GetAODVStatistics() << '/';
    Tstats << aodvProto->GetTrafficStatistics() << '/';
    TLRNstats << aodvProto->GetLrnTrafficStatistics() << '/';
    TRECstats << aodvProto->GetRecvTrafficStatistics() << '/';
    TLRNRECstats << aodvProto->GetRecvLrnTrafficStatistics() << '/';
    if (!qlearn) {
      avg_delay[interfaces.GetAddress(i)] = Time::FromInteger(int(aodvProto->AvgDelayAsFloat()), Time::NS);
    }
  }
  if (!in_test) {
    std::cout << Qstats.str() << std::endl;
    std::cout << "AODV TRAFFIC:   " << Astats.str() << std::endl;
    std::cout << "DATA TRAFFIC:   " << Tstats.str() << std::endl;
    std::cout << "LRN TRAFFIC:    " << TLRNstats.str() << std::endl;
    std::cout << "DATA REC TRAFFIC:   " << TRECstats.str() << std::endl;
    std::cout << "LRN REC TRAFFIC:    " << TLRNRECstats.str() << std::endl;
  }

  for (const auto & i : traffic_destinations) {
    std::cout << "\n    Observed average delay at " << i << " was " << avg_delay[i].As(Time::MS) << ".";
  }
  std::cout << "\n\n";

  for (const auto & j : traffic_sources) {
    for (const auto& i : packet_loss_per_neighb_from_src[j]) {
      if (std::find(traffic_destinations.begin(), traffic_destinations.end(), i.first.second) == traffic_destinations.end()) { } else {
        std::cout << "    Observed packet delivery ratio from source "<< j << " toward destination "<<
         i.first.second <<" going via " << i.first.first << " was " << i.second << ".\n";
      }
    }
  }
  std::cout << "\n";

  if (qlearn) {
    std::vector<int> QRoute = GetQRoute(interfaces.GetAddress(0),interfaces.GetAddress(numberOfNodes-1));
    std::cout << "    Observed (final) QRoute from " << interfaces.GetAddress(0) << " = ";
    for (const auto& i : QRoute) {
      std::cout << i << "-";
    }
    std::cout << "\n";

    QRoute = GetQRoute(interfaces.GetAddress(1),interfaces.GetAddress(numberOfNodes-1));
    std::cout << "    Observed (final) QRoute from " << interfaces.GetAddress(1) << " = ";
    for (const auto& i : QRoute) {
      std::cout << i << "-";
    }
    std::cout << "\n";

    QRoute = GetQRoute(interfaces.GetAddress(2),interfaces.GetAddress(numberOfNodes-1));
    std::cout << "    Observed (final) QRoute from " << interfaces.GetAddress(2) << " = ";
    for (const auto& i : QRoute) {
      std::cout << i << "-";
    }
    std::cout << "\n";
  }

  //wireshark traces kept showing 110, id have like 78 sent, but then we also have to count packets being sent back to node0 and then
  //being transmit again. With a number of L2 retrans, it came out to 11O. added this check tor remember that
  std::cout << "\n    Total nr of learning pkt sent from 1 needs to also count the nr dropped: "
            << Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol>(nodes.Get( 0 )->GetObject<Ipv4> ()->GetRoutingProtocol ())->m_nr_of_lrn_dropped
            << " or close to it..." << std::endl;

  Simulator::Destroy ();
  return 0;
}

void
QLearningBase::InterconnectQLearners() {
  if ((learning_phases || ideal) && qlearn) {
    for (unsigned int i = 0; i < numberOfNodes; i++) {
      auto qq = dynamic_cast<QLearner*> (   PeekPointer( QLearners.Get(i) ) );
      // Ptr<QLearner> qq = &dynamic_cast<QLearner&> (*( PeekPointer(nodes.Get(i)->GetApplication (j) ) ) );
      qq->SetOtherQLearners(nodes);
    }
  }
}

std::vector<int> QLearningBase::GetQRoute(Ipv4Address source, Ipv4Address dst) {
  if (!qlearn) { return std::vector<int>(); } // otherwise the dyn_cast is a seg fault right now
  Ipv4Address nextHop, currNode = source;
  std::vector<Ipv4Address> seen_addr;
  std::vector<int> result;

  while (currNode != Ipv4Address("1.1.1.1")){
    seen_addr.push_back(currNode);
    // While currNode isnt dst, find the interface that currNode address belongs to
    for (auto i = interfaces.Begin(); i != interfaces.End(); i++) {
      // When we find it, we also have found the node
      if ((*i).first->GetAddress(1,0).GetLocal() == currNode) {
        // find the q-learner of that node
        auto q = dynamic_cast<QLearner*> (   PeekPointer( QLearners.Get(i->first->GetObject<Node>()->GetId()) ) );
        // auto q = dynamic_cast<QLearner&> (*QLearners.Get(i->first->GetObject<Node>()->GetId()));
        if ( !q->CheckDestinationKnown( dst )  ){
          std::cout << "Destination isn't known, no qroute to be found / printed. " << std::endl;
          return result;
        } else {
          TrafficType t = traffic_string_to_traffic_type(traffic);
          nextHop = q->GetNextHop(dst, t);
          // bool next_hop_was_random = q->AllNeighboursBlacklisted(dst,t);
          // if (next_hop_was_random) {
          //   std::cout << "Next hop was random. What we've had so far is:\n";
          //   std::cout << "Loop found in the qroute, possibly hasnt converged properly : ";
          //   result.push_back(i->first->GetObject<Node>()->GetId());
          //   for (auto i : result) { std::cout << i << "->";} std::cout<<std::endl;
          // }
          if (std::find(seen_addr.begin(), seen_addr.end(), nextHop) != seen_addr.end()) {
            std::cout << "Loop found in the qroute, possibly hasnt converged properly : ";
            result.push_back(i->first->GetObject<Node>()->GetId());
            for (auto i : result) { std::cout << i << "->";} std::cout<<std::endl;
            return result;
          }
        }
        if (currNode != dst) {
          result.push_back(i->first->GetObject<Node>()->GetId());
        }
      }
    }
    // done with currNode, we move onto the nextHop for the following iteration
    if (currNode == dst) {
      currNode = Ipv4Address("1.1.1.1");
      result.push_back(numberOfNodes-1);
    } else if (nextHop == Ipv4Address(NO_NEIGHBOURS_REACHABLE_ROUTE_IP) ) {
      std::cout << "No route found in the Qrouting because there are no neighbours available, thats unfortunate." << std::endl;
      currNode = Ipv4Address("1.1.1.1");
    } else {
      currNode = nextHop;
    }
  }
  return result;
}

bool
QLearningBase::VerifyDelayLossResults (std::string& error_message) {
  if (!qlearn || test_results.GetLowerBoundDelay() == VALUE_UNUSED_INT_PARAMETER) { return true; }
  int avg_delay_at_dst;
  std::stringstream ss;
  auto qq = dynamic_cast<QLearner*> (   PeekPointer( QLearners.Get(numberOfNodes-1) ) );
  // auto qq = dynamic_cast<QLearner&> (*(QLearners.Get(numberOfNodes-1) ) );
  avg_delay_at_dst = int(qq->AvgDelayAsFloat());

  if (avg_delay_at_dst < test_results.GetLowerBoundDelay() ) {
    ss << "\nAvg delay was expected to be higher than " << test_results.GetLowerBoundDelay() << " but the real delay was " << avg_delay_at_dst << std::endl;
    error_message += ss.str();
    return false;
  } else {
    return true;
  }
}

bool
QLearningBase::VerifyQRouteResults(std::string& error_message) {
  std::vector<int> actual_q_route = GetQRoute(interfaces.GetAddress(0), interfaces.GetAddress(numberOfNodes-1));
  std::vector<int> expected_q_route = test_results.GetExpectedQRoute();

  if (expected_q_route.size() == 0) return true; // not intending to check the QRoute in this case...

  std::stringstream ss;
  bool ret = true;
  if (actual_q_route.size() != expected_q_route.size()) {
    ss  << "\nThe expected QRoute is not the same size as the actual QRoute... this means a problem. Size of actual = "
        << actual_q_route.size() << "  and size of expected =  " << expected_q_route.size() << std::endl;
    ret = false;
  } else {
    for (uint i = 0; i < actual_q_route.size(); i++) {
      if (actual_q_route[i] != expected_q_route[i] && expected_q_route[i] != Q_ROUTE_WILDCARD_VALUE) {
        ss  << "\nThe expected QRoute is not the same as the actual QRoute... this means a problem. They differ at index i = "
            << i << ". Value of actual there = "<< actual_q_route[i] << "  and value of expected = " << expected_q_route[i] << std::endl;
        ret = false;
      }
    }
  }

  if (!ret) {
    ss << "Actual route:  ";
    for (auto i : actual_q_route) {
      ss << i << " ->  ";
    } ss << "\nExpected route:  ";
    for (auto i : expected_q_route) {
      ss << i << " ->  ";
    } ss << "\n";
  }

  error_message += ss.str();
  return ret;
}

bool
QLearningBase::VerifyLearningResults(std::string& err) {
  bool ret = true;
  std::stringstream ss;
  auto expected_learning_nodes = test_results.GetExpectedLearning();
  auto expected_not_learning_nodes = test_results.GetExpectedNotLearning();

  for (const auto& ind : expected_learning_nodes ) {
    NS_ASSERT(traffic_destinations.size() == 1);
    if (!dynamic_cast<QLearner*> (   PeekPointer( QLearners.Get(ind) ) )->IsInLearningPhase(traffic_destinations.at(0))) {
      ss << "\nExpected " << ind << " to be in learning phase but it was not." << std::endl;
      ret = false;
    }
  }

  for (const auto& ind : expected_not_learning_nodes ) {
    if (dynamic_cast<QLearner*> (   PeekPointer( QLearners.Get(ind) ) )->IsInLearningPhase(traffic_destinations.at(0))) {
      ss << "\nExpected " << ind << " to not be in learning phase but it was." << std::endl;
      ret = false;
    }
  }

  err += ss.str();
  return ret;
}

bool
QLearningBase::VerifyBrokenResults(std::string& err) {
  auto expected_broken_nodes = test_results.GetExpectedBroken();
  bool ret = true;
  std::stringstream ss;

  for (int j = 0; j < int(QLearners.GetN()); j++) {
    if (std::find(expected_broken_nodes.begin(), expected_broken_nodes.end(), j) == expected_broken_nodes.end()) {
      for (const auto& k : {VOIP, WEB, VIDEO} ) {
        auto qq = dynamic_cast<QLearner*> (   PeekPointer( QLearners.Get(j) ) )->GetQTable(k);
        auto q_neighbours = qq.GetNeighbours();
        auto q_unavails = qq.GetUnavails();
        for (const auto& v : q_unavails) {
          for (auto g = 0; g < int(interfaces.GetN()); g++) {
          // for (auto i = interfaces.Begin(); i != interfaces.End(); i++) {
            if (interfaces.GetAddress(g) == v) {
              if (std::find(expected_broken_nodes.begin(), expected_broken_nodes.end(), g) == expected_broken_nodes.end() ){ // if it is marked unavail and not in expected broken...
                //error, unexpected broken
                ss << "\nExpected " << g << " to not be broken at node" << j << " but it was!" << std::endl;
                ret =  false;
              }
              break;
            }
          }
        }
      }
    }
  }

  for (const auto& i : test_results.GetExpectedBroken() ) {
    for (int j = 0; j < int(QLearners.GetN()); j++) {
      if (std::find(expected_broken_nodes.begin(), expected_broken_nodes.end(), j) == expected_broken_nodes.end()) {
        for (const auto& k : {VOIP, WEB, VIDEO} ) {
          auto qq = dynamic_cast<QLearner*> (   PeekPointer( QLearners.Get(j) ) )->GetQTable(k);
          auto q_neighbours = qq.GetNeighbours();
          auto q_unavails = qq.GetUnavails();
          if (std::find(q_neighbours.begin(), q_neighbours.end(), interfaces.GetAddress(i)) != q_neighbours.end() && j < i ) { // if it is a neighbour // & j < i -- do UPSTREAM ONLY!
            if (std::find(q_unavails.begin(), q_unavails.end(), interfaces.GetAddress(i)) == q_unavails.end() ) { // and if it is not marked unavail
              // error, unexpected not broken
              ss << "\nExpected " << i << " to be broken at node" << j << " but it was not!" << std::endl;
              ret =  false;
            }
          }
        }
      }
    }
  }
  err += ss.str();
  return ret;
}

bool
QLearningBase::VerifyAODVTrafficStatisticResults(std::string& error_message) {
  std::stringstream ss;
  int margin_of_error = 1;
  bool ret = true;
  for (unsigned int i = 0; i < numberOfNodes; i++) {
    Ptr<aodv::RoutingProtocol> aodvProto = Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol>(nodes.Get(i)->GetObject<Ipv4> ()->GetRoutingProtocol ());
    if (!(test_results.GetNodeTrafficExpectedResult(i, 1) == uint32_t(aodvProto->AODVStatistics())) && !(test_results.GetNodeTrafficExpectedResult(i, 1) == VALUE_UNUSED_INT_PARAMETER) ) {
      if ( test_results.GetNodeTrafficExpectedResult(i, 1) - margin_of_error <= uint32_t(aodvProto->AODVStatistics()) &&
           test_results.GetNodeTrafficExpectedResult(i, 1) + margin_of_error >= uint32_t(aodvProto->AODVStatistics()) )
      {
        ss << "\nAODV (" << uint32_t(aodvProto->AODVStatistics()) << ") not equal to expected result (" << test_results.GetNodeTrafficExpectedResult(i, 1)
            << ") but within margin of error (" << margin_of_error << ") for node" << i << ".";
        ret = true && ret;
      } else {
        ss  << "\nStatistics for AODV packets were incorrect for node " << i << " did not match our expectations.\n"
            << "Real:     " << uint32_t(aodvProto->AODVStatistics()) << std::endl
            << "Expected: " << test_results.GetNodeTrafficExpectedResult(i, 1);
        ret = false;
      }
    }
    error_message += ss.str();
    ss.str("");
  }
  return ret;
}

bool
QLearningBase::VerifyQTrafficStatisticResults(std::string& error_message, bool epsilon_test, bool breaking_test) {
  std::stringstream ss;
  int margin_of_error = 3;
  bool ret = true;
  for (unsigned int i = 0; i < numberOfNodes; i++) {
    Ptr<aodv::RoutingProtocol> aodvProto = Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol>(nodes.Get(i)->GetObject<Ipv4> ()->GetRoutingProtocol ());

    if (!(test_results.GetNodeTrafficExpectedResult(i, 0) == uint32_t(aodvProto->QStatistics())) && !(test_results.GetNodeTrafficExpectedResult(i, 0) == VALUE_UNUSED_INT_PARAMETER ) ) {
      if (epsilon_test) {
        // ignore the number of Q packets, just make sure the nr of traffic packets is accurate
        // BUT there should at least be as many QPackets as the ideal case, so do check that
        if (test_results.GetNodeTrafficExpectedResult(i, 0) > uint32_t(aodvProto->QStatistics())) {
          ret = false;
          ss  << "\nStatistics for Q packets were incorrect for node " << i << ", they did not match our expectations.\n"
              << "Real:     " << uint32_t(aodvProto->QStatistics()) << std::endl
              << "Expected: >=" << test_results.GetNodeTrafficExpectedResult(i, 0) << " NOTE: epsilon test , so exact number is unknown.";
        }
      }
      else if ( test_results.GetNodeTrafficExpectedResult(i, 0) - margin_of_error <= uint32_t(aodvProto->QStatistics()) &&
           test_results.GetNodeTrafficExpectedResult(i, 0) + margin_of_error >= uint32_t(aodvProto->QStatistics())  )
      {
        ss << "\nQ (" << uint32_t(aodvProto->QStatistics()) << ") not equal to expected result (" << test_results.GetNodeTrafficExpectedResult(i, 0)
            << ") but within margin of error (" << margin_of_error << ") for node" << i << ".";
        ret = true && ret;
      }
      else if (smaller_learning_traffic && learning_phases && (LearningGeneratingApplications.GetN() + DataGeneratingApplications.GetN()) == 2 ) {
        if (uint32_t(aodvProto->QStatistics()) > test_results.GetNodeTrafficExpectedResult(i, 0)) {
          ss << "\nQ (" << uint32_t(aodvProto->QStatistics()) << ") not equal to expected result (" << test_results.GetNodeTrafficExpectedResult(i, 0)
              << ") but larger than the expected value for node" << i << ". We use lower bounds for the t lrn rec so thats fine.";
          ret = true && ret;
        } else {
          ss << "\nQ (" << uint32_t(aodvProto->QStatistics()) << ") not equal to expected result (" << test_results.GetNodeTrafficExpectedResult(i, 0)
              << ") and not larger than the expected value for" << i << ". THIS IS NOT OK SO FALSE.";
          ret = false && ret;
        }
      }
      else {
        ss  << "\nStatistics for Q packets were incorrect for node " << i << ", they did not match our expectations.\n"
            << "Real:     " << uint32_t(aodvProto->QStatistics()) << std::endl
            << "Expected: " << test_results.GetNodeTrafficExpectedResult(i, 0);
        ret = false;
      }
    }
    error_message += ss.str();
    ss.str("");
  }
  return ret;
}

bool
QLearningBase::VerifyTrafficStatisticResults(std::string& error_message, bool epsilon_test, bool breaking_test) {
  std::stringstream ss;
  int margin_of_error = 3;
  bool ret = true;  for (unsigned int i = 0; i < numberOfNodes; i++) {
    Ptr<aodv::RoutingProtocol> aodvProto = Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol>(nodes.Get(i)->GetObject<Ipv4> ()->GetRoutingProtocol ());
    /* SENT DATA TRAFFIC */
    // std::cout << aodvProto->RecvTrafficStatistics() << ",";
    if (!(test_results.GetNodeTrafficExpectedResult(i, 2) == uint32_t(aodvProto->TrafficStatistics())) && !(test_results.GetNodeTrafficExpectedResult(i, 2) == VALUE_UNUSED_INT_PARAMETER) ) {
      if (breaking_test) {
        // because it just breaks at some point, then traffic stops
        if (  (test_results.GetNodeTrafficExpectedResult(i, 2) > uint32_t(aodvProto->TrafficStatistics()) ||
              test_results.GetNodeTrafficExpectedResult(i, 2) + margin_of_error < uint32_t(aodvProto->TrafficStatistics())) && !learning_phases) {
          ret = false;
          ss  << "\nStatistics for sent traffic packets were incorrect for node " << i << ", they did not match our expectations.\n"
              << "Real:     " << uint32_t(aodvProto->TrafficStatistics()) << std::endl
              << "Expected: [" << test_results.GetNodeTrafficExpectedResult(i, 2) << "," << test_results.GetNodeTrafficExpectedResult(i, 2)+margin_of_error
              << "] \nNOTE: breaking test , so exactly when sending stops is kinda iffy.";
        }
        else if ( (  test_results.GetNodeTrafficExpectedResult(i, 2) < uint32_t(aodvProto->TrafficStatistics()) ||
                    test_results.GetNodeTrafficExpectedResult(i, 2) + test_results.GetNodeTrafficExpectedResult(i, 2) / 10 < uint32_t(aodvProto->TrafficStatistics()) ) && learning_phases) {
          ret = true && ret;
          ss  << "\nStatistics for sent traffic packets (" << uint32_t(aodvProto->TrafficStatistics()) << ")were kind of incorrect for node " << i
              << ", they did not match our expectations exactly but were close enough("<< test_results.GetNodeTrafficExpectedResult(i, 2)
              << " +- 10 percent) not failing because of this \nNOTE: breaking test , so exactly when sending stops is kinda iffy.\n";
        }
      }
      else if (epsilon_test && test_case.GetMaxPackets() == 0) {
        if (  test_results.GetNodeTrafficExpectedResult(i, 2) > uint32_t(aodvProto->TrafficStatistics()) ||
              test_results.GetNodeTrafficExpectedResult(i, 2) + margin_of_error > uint32_t(aodvProto->TrafficStatistics())) {
          ret = false;
          ss  << "\nStatistics for sent traffic packets were incorrect for node " << i << ", they did not match our expectations.\n"
              << "Real:     " << uint32_t(aodvProto->TrafficStatistics()) << std::endl
              << "Expected: [" << test_results.GetNodeTrafficExpectedResult(i, 2) << "," << test_results.GetNodeTrafficExpectedResult(i, 2)+margin_of_error << "] NOTE: epsilon test & we did not specify a maximum, so exactly how many pkts are sent is not clear .";
        }
        else {
          ss << "\nSent traffic (" << uint32_t(aodvProto->TrafficStatistics()) << ") not equal to expected result (" << test_results.GetNodeTrafficExpectedResult(i, 2)
              << ") but they at least its higher and its epsilon + no maxpackets so its fine";
          ret = true && ret;
        }
      }
      else if ( (LearningGeneratingApplications.GetN() + DataGeneratingApplications.GetN()) == 1 && test_case.GetMaxPackets() != 0 && learning_phases ) {
        if (  test_results.GetNodeTrafficExpectedResult(i, 2) < uint32_t(aodvProto->TrafficStatistics()) ) {
          ss << "\nSent traffic (" << uint32_t(aodvProto->TrafficStatistics()) << ") not lower than expected results (" << test_results.GetNodeTrafficExpectedResult(i, 2)
              << ") but its higher and its only 1 app for both learning and traffic with maxpackets set";
          ret = true && ret;
        }
      }
      else if (learning_phases && test_case.GetMaxPackets() == 0) {
        if (test_results.GetNodeTrafficExpectedResult(i, 2) < uint32_t(aodvProto->TrafficStatistics())) {
          ss << "\nSent traffic (" << uint32_t(aodvProto->TrafficStatistics()) << ") not equal to expected result (" << test_results.GetNodeTrafficExpectedResult(i, 2)
              << ") but at least its higher and its learning phases + no maxpackets so its fine";
          ret = true && ret;
        }
      }
      else {
        ss  << "\nStatistics for sent traffic packets were incorrect for node " << i << " did not match our expectations.\n"
            << "Real:     " << uint32_t(aodvProto->TrafficStatistics()) << std::endl
            << "Expected: " << test_results.GetNodeTrafficExpectedResult(i, 2);
        ret = false && ret;
      }
    }

    /* RECV DATA TRAFFIC*/
    if (!(test_results.GetNodeTrafficExpectedResult(i, 3) == uint32_t(aodvProto->RecvTrafficStatistics())) && !(test_results.GetNodeTrafficExpectedResult(i, 3) == VALUE_UNUSED_INT_PARAMETER) ) {
      if ( ( (test_results.GetNodeTrafficExpectedResult(i, 3) + margin_of_error > uint32_t(aodvProto->RecvTrafficStatistics() ) ) &&
          ( /* underflow protection */(test_results.GetNodeTrafficExpectedResult(i, 3) < uint32_t(margin_of_error) ? 0 : test_results.GetNodeTrafficExpectedResult(i, 3) - margin_of_error)
          < uint32_t(aodvProto->RecvTrafficStatistics())) )  ) {
        ss << "\nRecv traffic (" << uint32_t(aodvProto->RecvTrafficStatistics()) << ") not equal to expected result (" << test_results.GetNodeTrafficExpectedResult(i, 3)
            << ") but within margin of error (" << margin_of_error << ") for node" << i << ".";
        ret = true && ret;
      } else if (((test_results.GetNodeTrafficExpectedResult(i, 3) + 1 == uint32_t(aodvProto->RecvTrafficStatistics())) ||
                (test_results.GetNodeTrafficExpectedResult(i, 3) - 1 == uint32_t(aodvProto->RecvTrafficStatistics()))) && test_case.GetBreakIds().size() > 1 ) {
        ss << "\nRecv traffic (" << uint32_t(aodvProto->RecvTrafficStatistics()) << ") not equal to expected result (" << test_results.GetNodeTrafficExpectedResult(i, 3)
            << ") but within margin of error (" << 1 << ") for node" << i << ". Break test (I think) so its iffy";
        ret = true && ret;
      } else if (epsilon_test && (test_results.GetNodeTrafficExpectedResult(i, 3) < uint32_t(aodvProto->RecvTrafficStatistics()))) {
        ss << "\nRecv traffic (" << uint32_t(aodvProto->RecvTrafficStatistics()) << ") not equal to expected result (" << test_results.GetNodeTrafficExpectedResult(i, 3)
            << ") for node" << i << " but it is higher and its epsilon test so that is ok.";
        ret = true && ret;
      } else if ( (LearningGeneratingApplications.GetN() + DataGeneratingApplications.GetN()) == 1 && test_case.GetMaxPackets() != 0 && learning_phases ) {
        if (  test_results.GetNodeTrafficExpectedResult(i, 3) < uint32_t(aodvProto->RecvTrafficStatistics()) ) {
          ss << "\nRecv traffic (" << uint32_t(aodvProto->RecvTrafficStatistics()) << ") not lower than expected results (" << test_results.GetNodeTrafficExpectedResult(i, 3)
              << ") but its higher and its only 1 app for both learning and traffic with maxpackets set so this is fine";
          ret = true && ret;
        }
      } else if (breaking_test && learning_phases && (LearningGeneratingApplications.GetN() + DataGeneratingApplications.GetN()) == 2) {
        // because it just breaks at some point, then traffic stops
        if (  test_results.GetNodeTrafficExpectedResult(i, 3) < uint32_t(aodvProto->RecvTrafficStatistics()) ) {
          ss  << "\nStatistics for recv traffic packets ("<< uint32_t(aodvProto->RecvTrafficStatistics()) <<")  were higher than expected ("<< test_results.GetNodeTrafficExpectedResult(i, 3) <<")for node " << i
              << "  \nNOTE: breaking test , so exactly when sending stops is kinda iffy && learning & two traffic sources so its allowed.\n";
          ret = true && ret;
        } else {
          ss  << "\nStatistics for RECEIVED traffic packets were lower than expected for node " << i << ".\n"
              << "Real:     " << uint32_t(aodvProto->RecvTrafficStatistics()) << std::endl
              << "Expected: >=" << test_results.GetNodeTrafficExpectedResult(i, 3) << " NOTE: breaking test , so exactly when sending stops is kinda iffy.\n";
          ret = false && ret;
        }
      } else if (breaking_test) {
        if ( test_results.GetNodeTrafficExpectedResult(i, 3) < uint32_t(aodvProto->RecvTrafficStatistics() ) ) {
          ret = true && ret;
          ss  << "\nStatistics for recv ("<< uint32_t(aodvProto->RecvTrafficStatistics()) <<") traffic packets were higher than expected ("
              << test_results.GetNodeTrafficExpectedResult(i, 3) <<") for node " << i << ", this is fine.\n";
        } else {
          ss  << "\nStatistics for RECEIVED traffic packets were incorrect for node " << i << ", they were lower than expected.\n"
              << "Real:     " << uint32_t(aodvProto->RecvTrafficStatistics()) << std::endl
              << "Expected: >=" << test_results.GetNodeTrafficExpectedResult(i, 3) << " NOTE: breaking test , so exactly when sending stops is kinda iffy.\n";
          ret = false && ret;
        }
      } else {
        ss  << "\nStatistics for received traffic packets were incorrect for node " << i << ", they did not match our expectations.\n"
            << "Real:     " << uint32_t(aodvProto->RecvTrafficStatistics()) << std::endl
            << "Expected: " << test_results.GetNodeTrafficExpectedResult(i, 3);
        ret = false;
      }
    }

    /* SENT LEARNING TRAFFIC */
    if (!(test_results.GetNodeTrafficExpectedResult(i, 4) == uint32_t(aodvProto->LrnTrafficStatistics())) && !(test_results.GetNodeTrafficExpectedResult(i, 4) == VALUE_UNUSED_INT_PARAMETER) ) {
      if (((test_results.GetNodeTrafficExpectedResult(i, 4) + 1 == uint32_t(aodvProto->LrnTrafficStatistics())) ||
          (test_results.GetNodeTrafficExpectedResult(i, 4) - 1 == uint32_t(aodvProto->LrnTrafficStatistics()))) && test_case.GetBreakIds().size() > 1 ) {
        ss << "\nSent learning traffic (" << uint32_t(aodvProto->LrnTrafficStatistics()) << ") not equal to expected result (" << test_results.GetNodeTrafficExpectedResult(i, 4)
            << ") but within margin of error (" << 1 << ") for node" << i << ". Break test (I think) so its iffy";
        ret = true && ret;
      } else if (epsilon_test && (test_results.GetNodeTrafficExpectedResult(i, 4) < uint32_t(aodvProto->LrnTrafficStatistics()))) {
        ss << "\nSent learning traffic (" << uint32_t(aodvProto->LrnTrafficStatistics()) << ") not equal to expected result (" << test_results.GetNodeTrafficExpectedResult(i, 4)
            << ") for node" << i << " but it is higher and its epsilon test so that is ok.";
        ret = true && ret;
      } else if (learning_phases) {
        if (test_results.GetNodeTrafficExpectedResult(i, 4) < uint32_t(aodvProto->LrnTrafficStatistics())) {
          ss << "\nSent learning traffic (" << uint32_t(aodvProto->LrnTrafficStatistics()) << ") not equal to expected result (" << test_results.GetNodeTrafficExpectedResult(i, 4)
              << ") "<< " at node" << i <<" but at least its higher and its learning phases so its fine";
          if ((test_results.GetNodeTrafficExpectedResult(i, 4) +1) * 15 /*arbitrary :)*/ < uint32_t(aodvProto->LrnTrafficStatistics())) {
            ss <<  "actually this much difference (Upperbound expected=  " << test_results.GetNodeTrafficExpectedResult(i, 4) << "  * 15, real =  " << uint32_t(aodvProto->LrnTrafficStatistics())
                << " ) i cannot allow. Setting to FALSE\n";
            ret = false && ret;
          } else {
            ret = true && ret;
          }
        } else {
          ss << "\nSent learning traffic (" << uint32_t(aodvProto->LrnTrafficStatistics()) << ") lower than expected result (" << test_results.GetNodeTrafficExpectedResult(i, 4)
            << ") at node" << i << " which is unfortunate. Higher would have been fine...\n";
          ret = false && ret;
        }
      } else {
        ss  << "\nStatistics for Sent learning traffic packets were incorrect for node " << i << ", they did not match our expectations.\n"
            << "Real:     " << uint32_t(aodvProto->LrnTrafficStatistics()) << std::endl
            << "Expected: " << test_results.GetNodeTrafficExpectedResult(i, 4);
        ret = false;
      }
    }

    /* RECV LEARNING TRAFFIC */
    if (!(test_results.GetNodeTrafficExpectedResult(i, 5) == uint32_t(aodvProto->RecvLrnTrafficStatistics())) && !(test_results.GetNodeTrafficExpectedResult(i, 5) == VALUE_UNUSED_INT_PARAMETER) ) {
      if (((test_results.GetNodeTrafficExpectedResult(i, 5) + 1 == uint32_t(aodvProto->RecvLrnTrafficStatistics())) ||
          (test_results.GetNodeTrafficExpectedResult(i, 5) - 1 == uint32_t(aodvProto->RecvLrnTrafficStatistics()))) && test_case.GetBreakIds().size() > 1 ) {
        ss << "\nRecv learning traffic (" << uint32_t(aodvProto->RecvLrnTrafficStatistics()) << ") not equal to expected result (" << test_results.GetNodeTrafficExpectedResult(i, 5)
            << ") but within margin of error (" << 1 << ") for node" << i << ". Break test (I think) so its iffy";
        ret = true && ret;
      } else if (epsilon_test && (test_results.GetNodeTrafficExpectedResult(i, 5) < uint32_t(aodvProto->RecvLrnTrafficStatistics()))) {
        ss << "\nRecv learning traffic (" << uint32_t(aodvProto->RecvLrnTrafficStatistics()) << ") not equal to expected result (" << test_results.GetNodeTrafficExpectedResult(i, 5)
            << ") for node" << i << " but it is higher and its epsilon test so that is ok.";
        ret = true && ret;
      } else if (learning_phases) {
        if (test_results.GetNodeTrafficExpectedResult(i, 5) < uint32_t(aodvProto->RecvLrnTrafficStatistics())) {
          ss << "\nRecv learning traffic (" << uint32_t(aodvProto->RecvLrnTrafficStatistics()) << ") not equal to expected result (" << test_results.GetNodeTrafficExpectedResult(i,5)
              << " at node" << i << ") but at least its higher and its learning phases so its fine";
          if ((test_results.GetNodeTrafficExpectedResult(i, 5)+1) * 15 /*arbitrary :)*/ < uint32_t(aodvProto->RecvLrnTrafficStatistics())) {
            ss <<  "actually this much difference (Upperbound expected=  " << test_results.GetNodeTrafficExpectedResult(i, 5) << "  * 15, real =  " << uint32_t(aodvProto->RecvLrnTrafficStatistics())
                << " ) i cannot allow. Setting to FALSE\n";
            ret = false && ret;
          } else {
            ret = true && ret;
          }
        } else {
          ss << "\nRecv learning traffic (" << uint32_t(aodvProto->RecvLrnTrafficStatistics()) << ") not equal to expected result (" << test_results.GetNodeTrafficExpectedResult(i, 5) << ")"
            << " at node" << i << " and instead its lower, we were expecting it to be higher (using lowerbounds). SETTING FALSE.";
          ret = false && ret;
        }
      } else {
        ss  << "\nStatistics for Recv learning traffic packets were incorrect for node " << i << ", they did not match our expectations.\n"
            << "Real:     " << uint32_t(aodvProto->RecvLrnTrafficStatistics()) << std::endl
            << "Expected: " << test_results.GetNodeTrafficExpectedResult(i, 5);
        ret = false;
      }
    }
    error_message += ss.str();
    ss.str("");
  }
  return ret;
}

void QLearningBase::ExecuteEvent(const TestEvent& t) {
  std::cout << "[Event] At time t = " << Simulator::Now().GetSeconds() << " : " << t.PrettyPrint();
  switch( t.et ) {
    case JITTER:
      nodes.Get(t.node_id)->GetObject<Ipv4L3Protocol> ()->SetDelay(t.new_value);
      nodes.Get(t.node_id)->GetObject<Ipv4L3Protocol> ()->SetJitter(t.new_value);
      break;
    case DELAY:
      nodes.Get(t.node_id)->GetObject<Ipv4L3Protocol> ()->SetDelay(t.new_value);
      break;
    case BREAK:
      nodes.Get (t.node_id)->GetObject<MobilityModel> ()->SetPosition(Vector (1e5, 1e5, 1e5));
      // Simulator::Schedule (Seconds (t.t), &MobilityModel::SetPosition, nodes.Get (t.node_id)->GetObject<MobilityModel> (), );
      break;
    case PACKET_LOSS:
      nodes.Get(t.node_id)->GetObject<Ipv4L3Protocol> ()->SetLoss(t.new_value/100);
      break;
    case TRAFFIC:
      std::cout << t.PrettyPrint() << std::endl;
      break;
    case MOVE:
      NS_FATAL_ERROR("not yet implemented.");
      break;
    default:
      NS_FATAL_ERROR("test for unreachable (ideally) code.");
  }
}

bool QLearningBase::RunTest(std::string& err) {
  Simulator::Schedule (Seconds (1), &QLearningBase::ReportProgress, this);

  CreateAndPlaceNodes ();
  CreateDevices ();
  InstallInternetStack ();
  InstallApplications ();

  Simulator::Stop (Seconds (totalTime));

  InterconnectQLearners();

  int delay = 30;
  std::vector<int> reversed_break_ids = test_case.GetBreakIds();
  std::reverse(reversed_break_ids.begin(), reversed_break_ids.end());
  for (const auto& i : reversed_break_ids) {
    // std::cout << "breaking " << i << " at " << delay << std::endl;
    Ptr<MobilityModel> mob = nodes.Get (i)->GetObject<MobilityModel> ();
    Simulator::Schedule (Seconds (delay), &MobilityModel::SetPosition, mob, Vector (1e5, 1e5, 1e5));
    if (test_case.GetVaryingBreakTimes()) {
      delay += 30;
    }
  }

  for (const auto& i : test_case.GetEvents()) {
    if (i.et == TRAFFIC) {
      HandleTrafficEvent(i);
      if (qlearn) { dynamic_cast<QLearner*> ( PeekPointer( QLearners.Get (i.node_id) ) )->SetMyTrafficDst(interfaces.GetAddress(i.node_rx)); }
    } else {
      Simulator::Schedule (Seconds (i.t), &QLearningBase::ExecuteEvent, this, i);
    }
  }

  //Because only here do we know all the traffic destinations (copy is also at the other ::Run)
  if (qlearn) {
    for (unsigned int j = 0; j < numberOfNodes; j++) {
      auto qq = dynamic_cast<QLearner*> ( PeekPointer( QLearners.Get (j) ) );
      qq->InitializeLearningPhases(traffic_destinations);
      if (!traffic.compare("ping") || !traffic.compare("udp-echo")) { qq->InitializeLearningPhases(traffic_sources); }
      qq->SetTrafficSources(traffic_sources);
      qq->SetTrafficDestinations(traffic_destinations);
    }
  } else {
    for (unsigned int i = 0; i < numberOfNodes; i++) {
      Ptr<aodv::RoutingProtocol> aodvProto = Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol>(nodes.Get(i)->GetObject<Ipv4> ()->GetRoutingProtocol ());
      aodvProto->SetTrafficDestinations(traffic_destinations);
      aodvProto->SetAttribute("OutputData", BooleanValue( std::find(traffic_destinations.begin(), traffic_destinations.end(), interfaces.GetAddress(i) ) != traffic_destinations.end() ) );
    }
  }

  if ((qlearn && printRoutes) || ( m_type_of_run == "q" )) {
    std::stringstream filename;
    filename << "qroute_t950_"  << test_case_filename;
    Simulator::Schedule (Seconds (950), &QLearningBase::PrintQRouteToFile, this, filename.str(), interfaces.GetAddress(0),interfaces.GetAddress(numberOfNodes-1));
    filename.str("");
    filename << "qroute_t1950_" << test_case_filename;
    Simulator::Schedule (Seconds (1950), &QLearningBase::PrintQRouteToFile, this, filename.str(), interfaces.GetAddress(0),interfaces.GetAddress(numberOfNodes-1));
    filename.str("");
    filename << "qroute_t2950_" << test_case_filename;
    Simulator::Schedule (Seconds (2950), &QLearningBase::PrintQRouteToFile, this, filename.str(), interfaces.GetAddress(0),interfaces.GetAddress(numberOfNodes-1));
    filename.str("");
    filename << "qroute_t4450_" << test_case_filename;
    Simulator::Schedule (Seconds (3450), &QLearningBase::PrintQRouteToFile, this, filename.str(), interfaces.GetAddress(0),interfaces.GetAddress(numberOfNodes-1));
    filename.str("");

  } else if (m_type_of_run == "aodv") {
    std::stringstream filename;
    filename << "aodv_t950_" << test_case_filename;
    Ptr<OutputStreamWrapper> routingStream_950 = Create<OutputStreamWrapper> (filename.str(), std::ios::out);
    filename.str("");
    filename << "aodv_t1950_" << test_case_filename;
    Ptr<OutputStreamWrapper> routingStream_1950 = Create<OutputStreamWrapper> (filename.str(), std::ios::out);
    filename.str("");
    filename << "aodv_t2950_" << test_case_filename;
    Ptr<OutputStreamWrapper> routingStream_2950 = Create<OutputStreamWrapper> (filename.str(), std::ios::out);
    filename.str("");
    filename << "aodv_t4450_" << test_case_filename;
    Ptr<OutputStreamWrapper> routingStream_3450 = Create<OutputStreamWrapper> (filename.str(), std::ios::out);
    filename.str("");

    aodv.PrintRoutingTableAllAt (Seconds (950), routingStream_950);
    aodv.PrintRoutingTableAllAt (Seconds (1950), routingStream_1950);
    aodv.PrintRoutingTableAllAt (Seconds (2950), routingStream_2950);
    aodv.PrintRoutingTableAllAt (Seconds (3450), routingStream_3450);
  } else if ( m_type_of_run == "qosq" ) {
    std::stringstream filename;
    filename << "qosqroute_t950_" << test_case_filename;
    Simulator::Schedule (Seconds (950 ), &QLearningBase::PrintQRouteToFile, this, filename.str(), interfaces.GetAddress(0),interfaces.GetAddress(numberOfNodes-1));
    filename.str("");
    filename << "qosqroute_t1950_" << test_case_filename;
    Simulator::Schedule (Seconds (1950), &QLearningBase::PrintQRouteToFile, this, filename.str(), interfaces.GetAddress(0),interfaces.GetAddress(numberOfNodes-1));
    filename.str("");
    filename << "qosqroute_t2950_" << test_case_filename;
    Simulator::Schedule (Seconds (2950), &QLearningBase::PrintQRouteToFile, this, filename.str(), interfaces.GetAddress(0),interfaces.GetAddress(numberOfNodes-1));
    filename.str("");
    filename << "qosqroute_t4450_" << test_case_filename;
    Simulator::Schedule (Seconds (3450), &QLearningBase::PrintQRouteToFile, this, filename.str(), interfaces.GetAddress(0),interfaces.GetAddress(numberOfNodes-1));
    filename.str("");



  }

  Simulator::Run ();

  if (qlearn && m_type_of_run == "n/a") {
    for (unsigned int i = 0; i < numberOfNodes; i++) {
      for (unsigned int j = 0; j < nodes.Get(i)->GetNApplications(); j++) {
        try {
          auto qq = dynamic_cast<QLearner*> (   PeekPointer( QLearners.Get(j) ) );
          // auto qq = dynamic_cast<QLearner&> (*(nodes.Get(i)->GetApplication (j))); -- this causes a strange seg fault
          if (i >= 99999) { //q table print at end of test run
            std::cout << qq->PrintQTable(traffic_string_to_traffic_type(traffic)) << std::endl;
          }
        } catch (const std::bad_cast &) {
          // do nothing, keep searching
        }
      }
    }
  }

  // if needed, check if total nr of learning pkts sent (in wireshark) match lrn_traffic_sent + lrn_traffic_recv - this nr
  // std::cout << "Total nr of learning pkt sent from 1 needs to also count the nr dropped: "
  //           << Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol>(nodes.Get( 0 )->GetObject<Ipv4> ()->GetRoutingProtocol ())->m_nr_of_lrn_dropped
  //           << " or close to it..." << std::endl;

  if (m_type_of_run != "n/a") {
    NS_ASSERT(m_type_of_run == "q" || m_type_of_run == "qosq" || m_type_of_run == "aodv" );
    std::stringstream filename;
    filename << m_type_of_run << "ALGORITHM_OUTPUT_"<< test_case_filename;
    Ptr<OutputStreamWrapper> output_strm = Create<OutputStreamWrapper> (filename.str(), std::ios::out);

    std::stringstream Astats, Qstats,Tstats,TLRNstats,TRECstats,TLRNRECstats;
    std::map<Ipv4Address,Time> avg_delay;
    std::map<Ipv4Address,std::vector<std::pair<std::pair<Ipv4Address,Ipv4Address>,float> > > packet_loss_per_neighb_from_src;
    Qstats << "node0(src)/";
    for (unsigned int i = 1; i < numberOfNodes-1; i++) {
      Qstats << "node" << i << "/";
    }
    Qstats << "node" << numberOfNodes-1 <<"(dst)/" << std::endl;
    if (qlearn) {
      for (unsigned int i = 0; i < numberOfNodes; i++) {
        auto qq = dynamic_cast<QLearner*> (   PeekPointer( QLearners.Get(i) ) );
        if (std::find(traffic_destinations.begin(), traffic_destinations.end(), interfaces.GetAddress(i)) != traffic_destinations.end()) {
          avg_delay[interfaces.GetAddress(i)] = Time::FromInteger(int(qq->AvgDelayAsFloat()), Time::NS);
        }
        if (std::find(traffic_sources.begin(), traffic_sources.end(), interfaces.GetAddress(i)) != traffic_sources.end()) {
          packet_loss_per_neighb_from_src[interfaces.GetAddress(i)] = qq->GetPacketLossPerNeighb();
        }
        qq->FinaliseQTables(traffic_string_to_traffic_type("trafficA"));
        qq->FinaliseQTables(traffic_string_to_traffic_type("trafficB"));
        qq->FinaliseQTables(traffic_string_to_traffic_type("trafficC"));
        Qstats << qq->GetStatistics() << '/';

        // for (unsigned int j = 0; j < nodes.Get(i)->GetNApplications(); j++) {
        //   try {
        //     auto qq = dynamic_cast<QLearner&> (*(nodes.Get(i)->GetApplication (j)));
        //     if (std::find(traffic_destinations.begin(), traffic_destinations.end(), interfaces.GetAddress(i)) != traffic_destinations.end()) {
        //       avg_delay[interfaces.GetAddress(i)] = Time::FromInteger(int(qq.AvgDelayAsFloat()), Time::NS);
        //     }
        //     if (std::find(traffic_sources.begin(), traffic_sources.end(), interfaces.GetAddress(i)) != traffic_sources.end()) {
        //       packet_loss_per_neighb_from_src[interfaces.GetAddress(i)] = qq.GetPacketLossPerNeighb();
        //     }
        //     qq.FinaliseQTables(traffic_string_to_traffic_type("trafficA"));
        //     qq.FinaliseQTables(traffic_string_to_traffic_type("trafficB"));
        //     qq.FinaliseQTables(traffic_string_to_traffic_type("trafficC"));
        //     Qstats << qq.GetStatistics() << '/';
        //   } catch (const std::bad_cast &) {
        //     // do nothing, keep searching
        //   }
        // }

      }
    }
    Qstats << std::endl << "QLRN TRAFFIC:   ";
    for (unsigned int i = 0; i < numberOfNodes; i++) {
      Ptr<aodv::RoutingProtocol> aodvProto = Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol>(nodes.Get(i)->GetObject<Ipv4> ()->GetRoutingProtocol ());
      Qstats << aodvProto->GetQStatistics() << '/';
      Astats << aodvProto->GetAODVStatistics() << '/';
      Tstats << aodvProto->GetTrafficStatistics() << '/';
      TLRNstats << aodvProto->GetLrnTrafficStatistics() << '/';
      TRECstats << aodvProto->GetRecvTrafficStatistics() << '/';
      TLRNRECstats << aodvProto->GetRecvLrnTrafficStatistics() << '/';
      if (!qlearn) {
        avg_delay[interfaces.GetAddress(i)] = Time::FromInteger(int(aodvProto->AvgDelayAsFloat()), Time::NS);
      }
    }
    *(output_strm->GetStream ()) << Qstats.str() << std::endl;
    *(output_strm->GetStream ()) << "AODV TRAFFIC:   " << Astats.str() << std::endl;
    *(output_strm->GetStream ()) << "DATA TRAFFIC:   " << Tstats.str() << std::endl;
    *(output_strm->GetStream ()) << "LRN TRAFFIC:    " << TLRNstats.str() << std::endl;
    *(output_strm->GetStream ()) << "DATA REC TRAFFIC:   " << TRECstats.str() << std::endl;
    *(output_strm->GetStream ()) << "LRN REC TRAFFIC:    " << TLRNRECstats.str() << std::endl;

    for (const auto & i : traffic_destinations) {
      *(output_strm->GetStream ()) << "Observed average delay at " << i << " was " << avg_delay[i].As(Time::MS) << ".";
    }
    *(output_strm->GetStream ())<< "\n";

    for (const auto & j : traffic_sources) {
      for (const auto& i : packet_loss_per_neighb_from_src[j]) {
        if (std::find(traffic_destinations.begin(), traffic_destinations.end(), i.first.second) == traffic_destinations.end()) { } else {
          *(output_strm->GetStream ()) << "Observed packet delivery ratio from source "<< j << " toward destination "
                      << i.first.second <<" going via " << i.first.first << " was " << i.second << ".\n";
        }
      }
    }

    if (qlearn) {
      std::vector<int> QRoute = GetQRoute(interfaces.GetAddress(0),interfaces.GetAddress(numberOfNodes-1));
      *(output_strm->GetStream ()) << "    Observed (final) QRoute from " << interfaces.GetAddress(0) << " = ";
      for (const auto& i : QRoute) {
        *(output_strm->GetStream ()) << i << "-";
      }
      *(output_strm->GetStream ()) << "\n";

      QRoute = GetQRoute(interfaces.GetAddress(1),interfaces.GetAddress(numberOfNodes-1));
      *(output_strm->GetStream ()) << "    Observed (final) QRoute from " << interfaces.GetAddress(1) << " = ";
      for (const auto& i : QRoute) {
        *(output_strm->GetStream ()) << i << "-";
      }
      *(output_strm->GetStream ()) << "\n";

      QRoute = GetQRoute(interfaces.GetAddress(2),interfaces.GetAddress(numberOfNodes-1));
      *(output_strm->GetStream ()) << "    Observed (final) QRoute from " << interfaces.GetAddress(2) << " = ";
      for (const auto& i : QRoute) {
        *(output_strm->GetStream ()) << i << "-";
      }
      *(output_strm->GetStream ()) << "\n";
    }

    //wireshark traces kept showing 110, id have like 78 sent, but then we also have to count packets being sent back to node0 and then
    //being transmit again. With a number of L2 retrans, it came out to 11O. added this check tor remember that
    // *(output_strm->GetStream ()) << "\n    Total nr of learning pkt sent from 1 needs to also count the nr dropped: "
    // << Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol>(nodes.Get( 0 )->GetObject<Ipv4> ()->GetRoutingProtocol ())->m_nr_of_lrn_dropped
    // << " or close to it..." << std::endl;
  }
  bool VerifyTraffic = true, VerifyQTraffic = true, VerifyAODV = true, VerifyBroken = true, VerifyLearning = true, VerifyQroute = true, VerifyDelayLoss = true;
  if (!test_results.IsLoaded()) {
    return true;
  } else {
    VerifyTraffic = VerifyTrafficStatisticResults(err, test_case.GetEps() != 0, test_case.GetBreakIds().size() != 0);
    VerifyQTraffic = VerifyQTrafficStatisticResults(err, test_case.GetEps() != 0, test_case.GetBreakIds().size() != 0);
    VerifyAODV = VerifyAODVTrafficStatisticResults(err);
    VerifyBroken = VerifyBrokenResults (err);
    VerifyLearning = VerifyLearningResults (err);
    VerifyQroute = VerifyQRouteResults (err);
    VerifyDelayLoss = VerifyDelayLossResults (err);
  }

  Simulator::Destroy ();
  Names::Clear();
  return VerifyTraffic && VerifyQTraffic && VerifyAODV && VerifyBroken && VerifyLearning && VerifyQroute && VerifyDelayLoss;

}

void QLearningBase::PrintLocationsToFile (std::string filename, bool ignore_broken = false)
{
    Ptr<OutputStreamWrapper> out = Create<OutputStreamWrapper> (filename, std::ios::out);
    for (unsigned int i = 0; i < nodes.GetN (); i++) {
      if (i != node_to_break || ignore_broken)
        {
          *(out->GetStream ()) << nodes.Get(i)->GetObject<MobilityModel>()->GetPosition().x << "   "
              << nodes.Get(i)->GetObject<MobilityModel>()->GetPosition().y << "   "
              << ns3::Names::FindName (nodes.Get(i))  << "   " << interfaces.GetAddress (i) <<"/"<< ns3::Names::FindName (nodes.Get(i))<< std::endl;
        }
    }
}

void
QLearningBase::CreateAndPlaceNodes ()
{
  double xmax = 300;
  double ymax = 300;
  // creation
  nodes.Create (numberOfNodes);
  Names::Add ("OrigTrafficSource", nodes.Get (0));
  for (uint32_t i = 1; i < numberOfNodes-1; ++i)
     {
      std::ostringstream os;
      os << "node-" << i;
      Names::Add (os.str (), nodes.Get (i));
     }
  Names::Add ("OrigTrafficDest", nodes.Get (numberOfNodes-1));

  // mobility
  mobility = MobilityHelper();
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

  if (!(fixLocs || test_case.IsLoaded())) {
    Ptr<RandomRectanglePositionAllocator> positionAlloc = CreateObject<RandomRectanglePositionAllocator> ();
    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
    x->SetAttribute ("Min", DoubleValue(1.0));
    x->SetAttribute ("Max", DoubleValue(xmax));
    Ptr<UniformRandomVariable> y = CreateObject<UniformRandomVariable> ();
    y->SetAttribute ("Min", DoubleValue(1.0));
    y->SetAttribute ("Max", DoubleValue(ymax));

    positionAlloc->SetX (x);
    positionAlloc->SetY (y);
    mobility.SetPositionAllocator(positionAlloc);

    mobility.Install (nodes);

    // set source to 0,0 and dst to 100,100
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
      "MinX", DoubleValue (0.0),
      "MinY", DoubleValue (0.0));
    mobility.Install (nodes.Get (0));

    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
      "MinX", DoubleValue (xmax),
      "MinY", DoubleValue (ymax));
    mobility.Install (nodes.Get (numberOfNodes - 1));
  } else if (fixLocs && !test_case.IsLoaded() ) {
    NS_ASSERT_MSG(numberOfNodes == 4, "number of nodes should be 5 or we wont be in the expected situation.");
    // set source to 0,0 and dst to 100,100
    /**
     * Distance = 90 is fine to bridge
     * but if we go higher they are no longer neighbours
     */

    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
      "MinX", DoubleValue (0.0),
      "MinY", DoubleValue (0.0));
    mobility.Install (nodes.Get (0));
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
      "MinX", DoubleValue (100.0),
      "MinY", DoubleValue (10.0));
    mobility.Install (nodes.Get (1));
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
      "MinX", DoubleValue (200.0),
      "MinY", DoubleValue (20.0));
    mobility.Install (nodes.Get (2));
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
      "MinX", DoubleValue (300.0),
      "MinY", DoubleValue (30.0));
    mobility.Install (nodes.Get (numberOfNodes - 1));
  } else if (test_case.IsLoaded() && !fixLocs) {
    uint8_t index = 0;
    for (const auto& location : test_case.GetLocations()) {
      mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
        "MinX", DoubleValue (location.first),
        "MinY", DoubleValue (location.second));
      mobility.Install (nodes.Get (index++));
    }
  }
}

void
QLearningBase::CreateDevices ()
{
  // Wifi : Default for now
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");

  // https://www.nsnam.org/wiki/HOWTO_add_basic_rates_to_802.11
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  // we create a channel object and associate it to our PHY layer object manager to make sure that all the PHY layer objects
  // created by the YansWifiPhyHelper share the same underlying channel, that is, they share the same wireless medium
  // and can communication and interfere:
  // phy.SetErrorRateModel ("ns3::NistErrorRateModel");
  phy.SetErrorRateModel ("ns3::YansErrorRateModel");
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;

  //Other possib : AARFWifiManager, for example
  // more / less data speed here ?
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
  devices = wifi.Install (phy, wifiMac, nodes);
  if (pcap)
    {
      phy.EnablePcapAll (std::string ("T"));
    }

  // AsciiTraceHelper ascii;
  // phy.EnableAsciiAll (ascii.CreateFileStream ("my.tr"));
}

void
QLearningBase::InstallInternetStack ()
{
  //  Perhaps later: IPv4List = multiple routing protocols in a priority order, first one to accept packet is chosen
  InternetStackHelper stack;

  // Ipv4GlobalRoutingHelper ipv4RoutingHelper;
  // stack.SetRoutingHelper (ipv4RoutingHelper);
  // stack.Install(nodes.Get(interference_node));

  aodv.Set ("OutputData", BooleanValue( false ) );
  if (qlearn) {
    aodv.Set ("HelloInterval", TimeValue(Seconds(100000)));
    aodv.Set ("GratuitousReply", BooleanValue(false));
  }

  stack.SetRoutingHelper (aodv);
  stack.Install(nodes);
  Ptr<aodv::RoutingProtocol> aodvProto = Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol>(nodes.Get(numberOfNodes-1)->GetObject<Ipv4> ()->GetRoutingProtocol ());
  aodvProto->SetAttribute("OutputData", BooleanValue( !qlearn && m_output_stats ) );
  //
  // stack.Get(numberOfNodes-1).Set("OutputData", BooleanValue( false )

  // for (int i = 0; i < int(numberOfNodes); i++) {
  //   if (i != interference_node) {
  //     stack.Install(nodes.Get(i));
  //   }
  // }

  Ipv4AddressHelper addresses;
  addresses.SetBase("10.1.1.0", "255.255.225.0");

  interfaces = addresses.Assign (devices);

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("aodv.routes", std::ios::out);
      Ptr<OutputStreamWrapper> routingStream2 = Create<OutputStreamWrapper> ("aodv2.routes", std::ios::out);
      Ptr<OutputStreamWrapper> routingStream3 = Create<OutputStreamWrapper> ("aodv3.routes", std::ios::out);

      // https://www.nsnam.org/doxygen/classns3_1_1_ipv4_routing_helper.html
      aodv.PrintRoutingTableAllAt (Seconds (25), routingStream);
      aodv.PrintRoutingTableAllAt (Seconds (40), routingStream2);
      // aodv.PrintRoutingTableAllAt (Seconds (2), routingStream3);
    }

  if (in_test && (pcap || m_type_of_run == "q" || traffic_x != "NOTHING") ) {
    PrintLocationsToFile("locations.txt", true);
  } else if (!in_test) {
    PrintLocationsToFile("locations.txt", true);
  }

  if (linkBreak)
      {
      //Give aodv some time to settle, then determine which node we will break (at time t = 900)
      Simulator::Schedule (Seconds (25),  &QLearningBase::DetermineNodeToBreak, this);
    }

}

void
QLearningBase::PrintQRouteToFile(std::string filename, Ipv4Address source, Ipv4Address dst) {
  Ptr<OutputStreamWrapper> out = Create<OutputStreamWrapper> (filename, std::ios::out);
  Ipv4Address nextHop, currNode = source;
  std::vector<Ipv4Address> seen_addr;

  *(out->GetStream ()) << "srcNode-" << currNode << "\n";
  *(out->GetStream ()) << "dstNode-" << interfaces.GetAddress(numberOfNodes-1) << "\n";

  // TrafficType t = traffic_string_to_traffic_type(traffic);

  std::vector<int> qroute_node_numbers = GetQRoute(source,dst);

  for ( uint i = 0; i < qroute_node_numbers.size() ; i++ ) {

    if (i < qroute_node_numbers.size()-1) {
      *(out->GetStream ()) << interfaces.GetAddress( qroute_node_numbers.at(i+1) ) << "," << qroute_node_numbers.at(i) << "\n";
    } else {
      // not sure what the purpose of this was ...
      // *(out->GetStream ()) << dynamic_cast<QLearner*>(PeekPointer(QLearners.Get(i) ) )->GetNextHop(dst, t)  << "," << qroute_node_numbers.at(i) << "vt\n";
    }
  }
  return;
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval, bool stop) {
  if (pktCount > 0 && !stop && socket != 0) {
    socket->Send (Create<Packet> (pktSize));
    Simulator::Schedule (pktInterval, &GenerateTraffic, socket, pktSize,pktCount-1, pktInterval, false);
  } else if ((socket != 0 && stop) || socket != 0 ) {
    socket->Close ();
    socket = 0;
  } else {
    //nothing
  }
}

void QLearningBase::InstallPing(ApplicationContainer& p) {
  V4PingHelper genHelper (interfaces.GetAddress (numberOfNodes-1));
  if (!in_test) {
    genHelper.SetAttribute ("Verbose", BooleanValue (true));
  }
  // genHelper.SetAttribute ("Interval", TimeValue(Seconds(1)));
  // genHelper.SetAttribute("", IntegerValue(256))
  p.Add(genHelper.Install (nodes.Get(0)));
}

void QLearningBase::InstallEventTraffic(ApplicationContainer& p,ApplicationContainer& s,
    int packet_size, int bitrate, int port, int nodeID_tx, int nodeID_rx, bool learning) {

  NS_ASSERT_MSG(uint(nodeID_rx) < numberOfNodes-1, "nodeID_rx too high or too low");
  NS_ASSERT_MSG(uint(nodeID_tx) < numberOfNodes-1, "nodeID_tx too high or too low");

  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", Address (InetSocketAddress (interfaces.GetAddress (nodeID_rx), port)));
  OnOffHelper      genHelper  ("ns3::UdpSocketFactory", Address (InetSocketAddress (interfaces.GetAddress (nodeID_rx), port)), learning);
  genHelper.SetConstantRate (bitrate, packet_size);
  p.Add(genHelper.Install (nodes.Get(nodeID_tx)));
  s.Add(sinkHelper.Install (nodes.Get (nodeID_rx)));
}

void QLearningBase::InstallTraffic(ApplicationContainer& p,ApplicationContainer& s, int packet_size, int bitrate, int port, bool learning) {
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", Address (InetSocketAddress (interfaces.GetAddress (numberOfNodes-1), port)));
  OnOffHelper      genHelper  ("ns3::UdpSocketFactory", Address (InetSocketAddress (interfaces.GetAddress (numberOfNodes-1), port)), learning);

  if (max_packets == 0 && test_case.IsLoaded() ) { max_packets = test_case.GetMaxPackets(); }

  genHelper.SetConstantRate (bitrate, packet_size);
  if (!learning) { genHelper.SetAttribute("MaxBytes", UintegerValue(max_packets*packet_size)); } // So maxPackets is respected.. I think ? - only do this for the traffic, laerning pkts is unlmtd

  p.Add(genHelper.Install (nodes.Get(0)));
  s.Add(sinkHelper.Install (nodes.Get (numberOfNodes-1)));
}


void QLearningBase::InstallUdpStream(ApplicationContainer& p, ApplicationContainer& s, int port_number) {
  /* USING UDP ECHO CLIENT AND SERVER */
  if (port_number == 0) port_number = 9999;

  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", Address (InetSocketAddress (interfaces.GetAddress (numberOfNodes-1), port_number)));
  UdpClientHelper genHelper (interfaces.GetAddress (numberOfNodes-1), port_number);

  uint32_t MaxPacketSize = TEST_UDP_NO_ECHO_PKT_SIZE;
  Time interPacketInterval = Seconds (2);

  if (test_case.IsLoaded()) { max_packets = test_case.GetMaxPackets();
    if (max_packets == 0) { max_packets = 2; } }

  genHelper.SetAttribute ("MaxPackets", UintegerValue (max_packets));
  genHelper.SetAttribute ("Interval", TimeValue (interPacketInterval));
  genHelper.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));

  p.Add(genHelper.Install (nodes.Get(0)));
  s.Add(sinkHelper.Install (nodes.Get (numberOfNodes-1)));
}

void QLearningBase::InstallUdpEcho(ApplicationContainer& p, ApplicationContainer& s, int port_number) {
  /* USING UDP ECHO CLIENT AND SERVER */
  if (port_number == 0) port_number = 9998;
  UdpEchoServerHelper sinkHelper (port_number);
  UdpEchoClientHelper genHelper (interfaces.GetAddress (numberOfNodes-1), port_number);

  uint32_t MaxPacketSize = 987;
  Time interPacketInterval = Seconds (1);

  if (test_case.IsLoaded()) { max_packets = test_case.GetMaxPackets();
    if (max_packets == 0) { max_packets = 2; } }

  genHelper.SetAttribute ("MaxPackets", UintegerValue (max_packets));
  genHelper.SetAttribute ("Interval", TimeValue (interPacketInterval));
  genHelper.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));

  p.Add(genHelper.Install (nodes.Get(0)));
  s.Add(sinkHelper.Install (nodes.Get (numberOfNodes-1)));
}

void
QLearningBase::InstallApplications ()
{
  if (!traffic.compare("ping")) {
    InstallPing(DataGeneratingApplications);
  }
  else if (!traffic.compare("trafficA")) {
    InstallTraffic(DataGeneratingApplications, DataSinkApplications, 300, 4800, PORT_NUMBER_TRAFFIC_A);
  }
  else if (!traffic.compare("trafficB")) {
    InstallTraffic(DataGeneratingApplications, DataSinkApplications, 300, 4800, PORT_NUMBER_TRAFFIC_B);
  }
  else if (!traffic.compare("trafficC")) {
    InstallTraffic(DataGeneratingApplications, DataSinkApplications, 300, 4800, PORT_NUMBER_TRAFFIC_C);
  }  else if (!traffic.compare("udp-weird")) {
    /* USING ONOFF APPLICATIONS FOR TRAFFIC A*/
    // Just too much traffic to make up anything useful?
    PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", Address (InetSocketAddress (interfaces.GetAddress (numberOfNodes-1), 9999)));
    OnOffHelper     genHelper  ("ns3::UdpSocketFactory", Address (InetSocketAddress (interfaces.GetAddress (numberOfNodes-1), 9999)));
    DataGeneratingApplications = genHelper.Install (nodes.Get(0));
    DataSinkApplications = sinkHelper.Install (nodes.Get (numberOfNodes-1));
  }
  else if (!traffic.compare("udp-but-no-echo-just-for-maxPkt")) {
    InstallUdpStream(DataGeneratingApplications, DataSinkApplications);
  }
  else if (!traffic.compare("udp-echo")) {
    InstallUdpEcho(DataGeneratingApplications, DataSinkApplications);
  }
  /* USING ONOFF APPLICATIONS FOR TRAFFIC B */
  // BASED ON http://www.eng.uwi.tt/depts/elec/staff/rvadams/sramroop/Report_parts/Sanjay%20Ramroop%20807004374%20ECNG%203020%20Report%20April%204th%20Final.pdf
  // AND https://www.researchgate.net/publication/228569278_Generic_modeling_of_multimedia_traffic_sources
  else if (!traffic.compare("voip")) {
    InstallVoIPTraffic(DataGeneratingApplications, DataSinkApplications);
  }
  else if (!traffic.compare("video")) {
    /* VIDEO */
    // Acceptable thresholds for voice are 1% loss, 30 ms jitter, 150 ms delay
    /*
    * Traces got from
    *  extras.springer.com/2007/978-1-4020-5565-2/publications/cnf_CCNC2004.pdf
    *  http://kom.aau.dk/~ff/documents/tracesim.pdf
    *  http://trace.eas.asu.edu/
    *
    *  Second (better?) src : http://www2.tkn.tu-berlin.de/research/trace/ltvt.html
    */
    PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", Address (InetSocketAddress (interfaces.GetAddress (numberOfNodes-1), 10001)));
    UdpTraceClientHelper genHelper (interfaces.GetAddress (numberOfNodes-1), 10001,"tracefile_small.dat");
    genHelper.SetAttribute("TraceLoop", BooleanValue(false));
    DataGeneratingApplications = genHelper.Install (nodes.Get(0));
    DataSinkApplications = sinkHelper.Install (nodes.Get (numberOfNodes-1));
  }
  else if (!traffic.compare("web")) {
    /* WEB */
    /** https://perso.ens-lyon.fr/thomas.begin/Publis/SIMUTools11.pdf
    * also https://github.com/sharan-naribole/PPBP-ns3/, did his own rework apparently which is not entirely similar
    * Link to code is in Conclusion, then reworked code for ns3-26 (was designed for ns3-13 or so)
    * For the PPBP generator, we select H = 0.7 as recommended in [5], T = 200 ms and r = 1 Mbps.  The packets  size  is  set  to  1470  bytes
    */
    PPBPHelper genHelper("ns3::UdpSocketFactory", Address (InetSocketAddress (interfaces.GetAddress (numberOfNodes-1), 9999)) );
    genHelper.SetAttribute("H", DoubleValue (0.7) ); //Hurst Parameter
    genHelper.SetAttribute("MeanBurstArrivals", StringValue ("ns3::ConstantRandomVariable[Constant=1]" ) ); //Hurst Parameter
    genHelper.SetAttribute("MeanBurstTimeLength", StringValue ("ns3::ConstantRandomVariable[Constant=0.02]") ); //Hurst Parameter
    DataGeneratingApplications = genHelper.Install (nodes.Get(0));
  }
  else if (traffic.find('/') != std::string::npos) {
    int learner_port_number = -1;
    std::stringstream stream(traffic);
    std::string segment;

    while(std::getline(stream, segment, '/')) {  }
    learner_port_number = traffic_string_to_port_number(segment);

    stream.clear();
    stream.str(traffic);

    int ctr=0;
    auto container_choice = [this] ( int ctr )->ApplicationContainer& {
      if ( ctr == 1 ) {
        return LearningGeneratingApplications;
      } else if ( ctr == 2) {
        return DataGeneratingApplications;
      } else {
        NS_FATAL_ERROR("Too many traffics"<< ctr <<"\n");
        return DataGeneratingApplications;
      }
    };

    if (!qlearn) {
      ctr = 2;
      while(std::getline(stream, segment, '/')) { }

      if      (segment == "voip") { InstallVoIPTraffic(container_choice(ctr), DataSinkApplications,learner_port_number, ctr == 1); }
      else if (segment == "ping") { InstallPing(container_choice(ctr)); }
      else if (segment == "trafficA") { InstallTraffic(container_choice(ctr), DataSinkApplications,300,4800,learner_port_number, ctr == 1 );}
      else if (segment == "trafficB") { InstallTraffic(container_choice(ctr), DataSinkApplications,300,4800,learner_port_number, ctr == 1); }
      else if (segment == "trafficC") { InstallTraffic(container_choice(ctr), DataSinkApplications,300,4800,learner_port_number, ctr == 1); }
      else if (segment == "udp-but-no-echo-just-for-maxPkt") { InstallUdpStream(container_choice(ctr), DataSinkApplications,learner_port_number); }
      else if (segment == "udp-echo") { InstallUdpEcho(container_choice(ctr), DataSinkApplications,learner_port_number); }
      else { NS_FATAL_ERROR ("Unrecognized traffic"); }

    } else {
      NS_ASSERT_MSG(learning_phases,"If not using learning phases, dont include a learning generator and a traffic generator.");
      while(std::getline(stream, segment, '/')) {
        ctr += 1;
        if      (segment == "voip") { InstallVoIPTraffic(container_choice(ctr), DataSinkApplications,learner_port_number, ctr == 1); }
        else if (segment == "ping") { InstallPing(container_choice(ctr)); }
        else if (segment == "trafficA") { InstallTraffic(container_choice(ctr), DataSinkApplications,300,4800,learner_port_number, ctr == 1 );}
        else if (segment == "trafficB") { InstallTraffic(container_choice(ctr), DataSinkApplications,300,4800,learner_port_number, ctr == 1); }
        else if (segment == "trafficC") { InstallTraffic(container_choice(ctr), DataSinkApplications,300,4800,learner_port_number, ctr == 1); }
        else if (segment == "udp-but-no-echo-just-for-maxPkt") { InstallUdpStream(container_choice(ctr), DataSinkApplications,learner_port_number); }
        else if (segment == "udp-echo") { InstallUdpEcho(container_choice(ctr), DataSinkApplications,learner_port_number); }
        else { NS_FATAL_ERROR ("Unrecognized traffic"); }
      }

      NS_ASSERT_MSG(DataGeneratingApplications.GetN() > 0, "We do need some application to generate traffic or it won't make any sense.");

      // In this case, we should freeze the traffic generating thing until the QLearner has converged to some values!
      try {
        dynamic_cast<OnOffApplication&> ( *(PeekPointer(DataGeneratingApplications.Get(0))) ).SetMaxBytes(FREEZE_ONOFFAPPLICATION_SENDING);
      } catch (std::exception & bc) {
        NS_FATAL_ERROR("We expect all traffic generators to be OnOffGenerators right now, due to the changes that were made to OnOffApplication to make it stop/start." << bc.what());
      }

      NS_ASSERT_MSG(ctr <= 2, "too many traffics specified!");
    }
  }
  else {
    NS_FATAL_ERROR("Traffictype was configured incorrectly");
  }

  NS_ASSERT_MSG(DataGeneratingApplications.GetN() == 1,"Not the expected number of real traffic generators!" << DataGeneratingApplications.GetN());

  NS_ASSERT_MSG(!qlearn || (qlearn && (LearningGeneratingApplications.GetN() == 1 || traffic.find('/') == std::string::npos) ), \
      "(qlrn) Not the expected number of learning traffic generators!" << LearningGeneratingApplications.GetN());

  NS_ASSERT_MSG(qlearn || (!qlearn && LearningGeneratingApplications.GetN() == 0 ), \
      "(non qlrn) Not the expected number of learning traffic generators!" << LearningGeneratingApplications.GetN());

  NS_ASSERT_MSG(!qlearn || (qlearn && DataSinkApplications.GetN() == 2) || traffic.find('/') == std::string::npos, \
      "Not the expected number of sinks. " << DataSinkApplications.GetN());

  NS_ASSERT_MSG(qlearn || (!qlearn && DataSinkApplications.GetN() == 1), \
      "Not the expected number of sinks. " << DataSinkApplications.GetN());

  DataGeneratingApplications.Start (Seconds (5)); //Issue with the sending ICMP destination unreachable was because default start time is 0, before the sink is started...
  LearningGeneratingApplications.Start (Seconds (5));
  DataGeneratingApplications.Stop (Seconds (totalTime-30));
  LearningGeneratingApplications.Stop (Seconds (totalTime-30));
  DataSinkApplications.Start (Seconds (4));
  DataSinkApplications.Stop (Seconds (totalTime-10));

  //These are always source / dst, so add them here
  // + any that might come from the handle traffic event
  traffic_destinations.push_back(interfaces.GetAddress (numberOfNodes-1));

  // if (!qlearn) {
  //   Ptr<aodv::RoutingProtocol> aodvProto_src = Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol>(nodes.Get(0)->GetObject<Ipv4> ()->GetRoutingProtocol ());
  //   Ptr<aodv::RoutingProtocol> aodvProto_dst = Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol>(nodes.Get(numberOfNodes-1)->GetObject<Ipv4> ()->GetRoutingProtocol ());
  //   aodvProto_dst->SetSrcAODV(aodvProto_src, interfaces.GetAddress(0));
  // }

  traffic_sources.push_back(interfaces.GetAddress (0));

  /* Q Learning application */
  if (qlearn) {
    qlrn = QLearnerHelper(eps, learning_rate, gamma, q_conv_thresh, rho, learn_more_threshold, in_test,
                          max_retry, ideal, learning_phases, m_qos_qlearning, m_output_stats, printQTables, metrics_back_to_src);

    QLearners = qlrn.Install (nodes);
    QLearners.Start (Seconds(4));

    for (const auto& i : test_case.GetQJitterIds()) {
      dynamic_cast<QLearner*> (   PeekPointer( QLearners.Get (i) ) )->SetDelay(test_case.GetNodeDelayValue(i) );
      dynamic_cast<QLearner*> (   PeekPointer( QLearners.Get (i) ) )->SetJittering(true);
    }
    for (const auto& i : test_case.GetQSlowIds()) {
      dynamic_cast<QLearner*> (   PeekPointer( QLearners.Get (i) ) )->SetDelay(test_case.GetNodeDelayValue(i) );
      dynamic_cast<QLearner*> (   PeekPointer( QLearners.Get (i) ) )->SetSlow(true);
    }

    if (LearningGeneratingApplications.GetN() > 0) {
      dynamic_cast<QLearner*> ( PeekPointer( QLearners.Get (0) ) )->SetLearningTrafficGen(LearningGeneratingApplications.Get(0));
    }
    if (DataGeneratingApplications.GetN() > 0) {
      dynamic_cast<QLearner*> ( PeekPointer( QLearners.Get (0) ) )->SetTrafficGen(DataGeneratingApplications.Get(0));
    }
    NS_ASSERT_MSG(DataGeneratingApplications.GetN() + LearningGeneratingApplications.GetN() >= 1,
      "We need to have a least SOME traffic generating applications otherwise what will there be to simulate??");
    auto qq = dynamic_cast<QLearner*> ( PeekPointer( QLearners.Get (0) ) );
    if (LearningGeneratingApplications.GetN() > 0) {
      qq->SetSmallLrnTraffic(smaller_learning_traffic);
    }
    qq->SetMyTrafficDst(interfaces.GetAddress(numberOfNodes-1));
  }

  for (const auto& i : test_case.GetL3JitterIds()) {
    nodes.Get(i)->GetObject<Ipv4L3Protocol> ()->SetDelay(test_case.GetNodeDelayValue(i));
    nodes.Get(i)->GetObject<Ipv4L3Protocol> ()->SetJitter(true);
  }

  for (const auto& i : test_case.GetL3SlowIds()) {
    nodes.Get(i)->GetObject<Ipv4L3Protocol> ()->SetDelay(test_case.GetNodeDelayValue(i));
  }

}

void QLearningBase::SetupAODVToDst() {
  NS_FATAL_ERROR("Test for unreachable code. This function should no longer be in use.");
  for (int i = numberOfNodes-2; i >= 0; i--) { // -2, dont need dst to RREQ itself
    Ptr<aodv::RoutingProtocol> aodvProto = Ipv4RoutingHelper::GetRouting <aodv::RoutingProtocol>(nodes.Get(i)->GetObject<Ipv4> ()->GetRoutingProtocol ());
    aodvProto->SendRequest(interfaces.GetAddress (numberOfNodes-1));

    Ptr<QLearner> qq = dynamic_cast<QLearner*> (   PeekPointer( QLearners.Get (i) ) );
    qq->AddDestination(Ipv4Address("255.255.255.255"), interfaces.GetAddress (numberOfNodes-1), Seconds(0));
  }
}

void
QLearningBase::CreateAnInterferer(int index) {
  NS_FATAL_ERROR("Test for unreachable code. This function should no longer be in use.");
  // OnOffHelper     genHelper  ("ns3::UdpSocketFactory", Address (InetSocketAddress (interfaces.GetAddress (numberOfNodes-1), 9999)));
  // DataGeneratingApplications = genHelper.Install (nodes.Get(0));
  // DataSinkApplications = sinkHelper.Install (nodes.Get (numberOfNodes-1));

  interferer = Socket::CreateSocket (nodes.Get (index), TypeId::LookupByName ("ns3::UdpSocketFactory"));
  InetSocketAddress interferingAddr = InetSocketAddress (Ipv4Address ("255.255.255.255"), 49000);
  interferer->SetAllowBroadcast (true);
  interferer->Connect (interferingAddr);
  Simulator::ScheduleWithContext (interferer->GetNode ()->GetId (),
                                  Seconds (3), &GenerateTraffic,
                                  interferer, 1000 /* pkt size*/ , 40000 /*num pkts */, MilliSeconds(1), false);

}

void QLearningBase::InstallVoIPTraffic(ApplicationContainer& p, ApplicationContainer& s, int port_number, bool learning, int node_tx, int _node_rx) {
  int node_rx = numberOfNodes -1;
  if (_node_rx != -1) {
    node_rx = _node_rx;
  }

  if (port_number == 0) { port_number = 10000; }

  max_packets = 0;
  if (test_case.IsLoaded() && !learning) {
    max_packets = test_case.GetMaxPackets();
  }

  /* VOICE */
  // use UDP for voice
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", Address (InetSocketAddress (interfaces.GetAddress (node_rx), port_number)));
  OnOffHelper     genHelper  ("ns3::UdpSocketFactory", Address (InetSocketAddress (interfaces.GetAddress (node_rx), port_number)), learning);

  // Acceptable thresholds for voice are 1% loss, 10 ms jitter, 150 ms delay
  // talk spurt -> silence. During talk spurt, every time a packet is ready (packetization time T) it is sent.
  // duration of spurt / silence is estimated by independent exponential laws of parameters a,b
  // Parameters found in paper(s):
  float a = 0.352;
  float b = 0.650;

  // during spurt, every 1/T time there is a packet being sent, this time is constant
  // choice of codec: G711 as was done in paper _1, and suggested in paper _2, there are other possibilities but this is highest quality, needs highest QoS!
  // G711 generates 64 Kbps of data out of the application
  // we need to choose some packetization time --
  // in the paper they use 20 ms -> every 20 ms we make a packet, 20 ms is 1/50 so 1280 bits or 160 bytes
  // Each Packet gets some overhead, this is 8 B for UDP header, 20 B Ip header and 12 B RTP header (which isnt in ns3 ?)
  // So we get packets of 172 B every 20 ms to compensate for the RTP header not being in ns-3
  genHelper.SetAttribute("PacketSize", UintegerValue ((node_tx == 0 ? 172 : 172) ) );
  genHelper.SetAttribute("MaxBytes", UintegerValue(max_packets*172)); // So maxPackets is respected.. I think ?
  // 172 B per 20 ms means 172 * 50 * 8 bits per second, or 68800 datarate
  genHelper.SetAttribute("DataRate", DataRateValue((node_tx == 0 ? 68800 : 68800) ) );

  //Above two params guarantee the IAT of 20 ms (right?)
  //Now, for the on-time and off-time...
  // ExponentialRandomVariable params: (mean, upperbound)
  Ptr<ExponentialRandomVariable> OnTime = CreateObject<ExponentialRandomVariable> ();
  OnTime->SetAttribute ("Mean", DoubleValue (a));

  Ptr<ExponentialRandomVariable> OffTime = CreateObject<ExponentialRandomVariable> ();
  OffTime->SetAttribute ("Mean", DoubleValue (b));

  genHelper.SetAttribute("OnTime", PointerValue(OnTime));
  genHelper.SetAttribute("OffTime", PointerValue(OffTime));

  // mean rate of sending packets across both ON and OFF states can be calculated by taking ratio of T(On) / T(Off) + T(On) and multiplying by 1/T but im not sure thats useful
  // duration of a call is exponentially distributed with mean c = 3 minutes
  // https://groups.google.com/forum/#!topic/ns-3-users/iG9Y3KBx29U //Vanhier
  p.Add(genHelper.Install (nodes.Get(node_tx)));

  // Ptr<OnOffApplication> oop = dynamic_cast<OnOffApplication*> (   PeekPointer( DataGeneratingApplications.Get(0) ) );
  // TraceConnectWithoutContext ("Tx", MakeCallback(&RoutingProtocol::PacketTrackingOutput , this));


  s.Add(sinkHelper.Install (nodes.Get (node_rx)));
}

}
