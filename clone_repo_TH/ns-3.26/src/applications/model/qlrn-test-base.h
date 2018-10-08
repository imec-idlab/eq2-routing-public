#include "ns3/aodv-module.h" /* aodv routing on nodes */
#include "ns3/mobility-module.h" /* makes STA mobile but we dont want any of that <-- needed for placement in grid */
#include "ns3/core-module.h" /* CommandLine (among others) */
#include "ns3/network-module.h" /* packets, sockets, links ? */
#include "ns3/internet-module.h" /* ipv4 / ipv6 / internet stack helper */
#include "ns3/point-to-point-module.h"    /* point to point = NIC / channel ctl */
#include "ns3/wifi-module.h" /* wifi stuff */
#include "ns3/v4ping-helper.h" /* Create a IPv4 ping application and associate it to a node. */
#include "ns3/ipv4-global-routing.h" /* for test */
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/q-learner-helper.h"
#include "ns3/q-learner.h"
#include "ns3/qos-q-learner.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/application-container.h"
#include "ns3/udp-echo-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/v4ping.h"
#include "ns3/qlrn-test.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ppbp-helper.h"
#include "ns3/make-functional-event.h"
#include "ns3/packet-sink.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#ifndef Q_LEARN_BASE_TEST_H_
#define Q_LEARN_BASE_TEST_H_

namespace ns3 {

class QLearningBase {
public:
  QLearningBase ();
  /// Configure script parameters, \return true on successful configuration
  bool Configure (int argc, char **argv);
  bool ConfigureTest (bool, bool, bool, bool, bool, unsigned int, std::string, std::string);
  /// Run simulation (with return int for error code, potentially)
  int Run ();
  /// Run test
  bool RunTest (std::string& error_message);

  /// Small function to report progress of time during Simulation
  void ReportProgress ();

  /// report statistics about control etc (will be called from the AODV I guess ?)
  void ReportStatistics () ;

  /// Verify test results for packets that we saw
  bool VerifyQRouteResults(std::string& error_message) ;
  bool VerifyTrafficStatisticResults(std::string& error_message, bool epsilon_test, bool breaking_test) ;
  bool VerifyQTrafficStatisticResults(std::string& error_message, bool epsilon_test, bool breaking_test) ;
  bool VerifyAODVTrafficStatisticResults(std::string& error_message) ;
  bool VerifyLearningResults(std::string& error_message);
  bool VerifyBrokenResults(std::string& error_message) ;
  bool VerifyDelayLossResults(std::string& error_message) ;

  int GetNoN() { return numberOfNodes; }
  ApplicationContainer GetDataGenerator() { return DataGeneratingApplications; }
  ApplicationContainer GetQLearners() { return QLearners; }

  void SetTotalTime (double t) { totalTime = t; }

  void NullInterfererSocket() { interferer = 0; }

  std::string StringTestInfo() { if (test_case.IsLoaded()) { return test_case.PrettyPrint(); } else { return "TEST CASE NOT LOADED.";} };

private:
  /// Number of nodes in the network (>= 2)
  uint32_t numberOfNodes;
  /// Simulation time, seconds
  double totalTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  /// Print routes if true
  bool printRoutes;
  /// Print qtables if true
  bool printQTables;
  /// Link break somewhere? (and unbreak?)
  bool linkBreak;
  bool linkUnBreak;
  /// To QLearn or not to QLearn ?
  bool qlearn;
  /// Nr of hops downstream from icmp source to sever the link
  unsigned int numHops;
  /// store ID of that node
  unsigned int node_to_break;
  /// set eps value
  float eps;
  /// set gamma value
  float gamma;
  /// set learning rate value
  float learning_rate;
  /// fix locations for testing / debugging
  bool fixLocs;
  /// kind of traffic
  std::string traffic;
  std::string traffic_x;
  /// ideal
  bool ideal;
  /// in_test bool disables output in test cases
  bool in_test;
  /// node that supplies iference for better/worse quality traffic
  int interference_node;
  /// to use or not to use the learning phases system
  bool learning_phases;
  /// qos qlearning enabler
  bool m_qos_qlearning;
  /// rho
  float rho;
  /// max_retry
  int max_retry;
  /// q conv thesh
  float q_conv_thresh;
  /// lrn more thresh
  float learn_more_threshold;

  /// to let us run multiple variations of same script with 1 script
  std::string m_type_of_run;

  ///max nr of pkts to send (if relevant, used to reproduce tests from cli)
  uint32_t max_packets;

  FlowMonitorHelper flowmonHelper;
  Ptr<FlowMonitor> flowmon;
  bool enable_flow_monitor;
  bool m_output_stats;
  bool metrics_back_to_src;
  bool smaller_learning_traffic;
  std::string test_case_filename;
  QLrnTest test_case;
  QLrnTestResult test_results;

  void ExecuteEvent(const TestEvent& t);

  //Helpers (reporting)
  MobilityHelper mobility;
  AodvHelper aodv;
  QLearnerHelper qlrn;

  // network
  NodeContainer nodes;
  NetDeviceContainer devices;
  Ipv4InterfaceContainer interfaces;

  //applications
  ApplicationContainer DataGeneratingApplications;
  ApplicationContainer LearningGeneratingApplications;
  ApplicationContainer DataSinkApplications;
  ApplicationContainer QLearners;

  // std::vector<ApplicationContainer> EventDataSinkApplications;
  // std::vector<ApplicationContainer> EventDataGeneratingApplications ;
  // std::vector<ApplicationContainer> EventLearningGeneratingApplications ;
  ApplicationContainer EventDataSinkApplications;
  ApplicationContainer EventDataGeneratingApplications ;
  ApplicationContainer EventLearningGeneratingApplications ;

  Ptr<Socket> interferer;
  std::vector<Ipv4Address> traffic_sources;
  std::vector<Ipv4Address> traffic_destinations;
private:
  void HandleTrafficEvent(TestEvent);

  void CreateAndPlaceNodes ();
  void CreateDevices ();
  void InstallInternetStack ();

  void InstallPing(ApplicationContainer&);
  void InstallVoIPTraffic(ApplicationContainer&, ApplicationContainer&, int = 0, bool = false, int node_tx = 0, int node_rx =-1);
  void InstallTraffic(ApplicationContainer&, ApplicationContainer&, int, int, int, bool= false);
  void InstallEventTraffic(ApplicationContainer&, ApplicationContainer&, int, int, int, int, int, bool= false);
  void InstallUdpStream(ApplicationContainer&, ApplicationContainer&, int = 0);
  void InstallUdpEcho(ApplicationContainer&, ApplicationContainer&, int = 0);

  void InstallApplications ();
  void ReportStr ( StringValue );
  void DetermineNodeToBreak (  );
  void UnBreakNode ( );
  void PrintLocationsToFile (std::string filename, bool);
  void PrintQRouteToFile(std::string filename, Ipv4Address,Ipv4Address);
  void CreateAnInterferer(int index);
  void SetupAODVToDst();
  void InterconnectQLearners();
  std::vector<int> GetQRoute(Ipv4Address,Ipv4Address);
};

}
#endif /* Q_LEARN_BASE_TEST_H_ */
