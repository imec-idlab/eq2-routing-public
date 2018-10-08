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
#include "ns3/udp-client-server-helper.h"
#include "ns3/applications-module.h"
#include "ns3/qlrn-test.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ppbp-helper.h"
#include "ns3/v4ping.h"

// #include "qlrn-test-base.h"

#include <iostream>
#include <cmath>

using namespace ns3;

// https://www.nsnam.org/doxygen/group__testing.html -- for other ASSERT stuff..

class QLearnerBasicShortTestCase : public QLearningBase {
public:
  QLearnerBasicShortTestCase ( ) : QLearningBase ("Testing basic setup & configuration with ICMP traffic [SHORT DURATION, 3 NODE]") {  }
  ~QLearnerBasicShortTestCase ( ) { }
private:
  void DoRun (void);
};

void QLearnerBasicShortTestCase::DoRun (void) {
  NS_TEST_ASSERT_MSG_EQ ( ConfigureTest ( true /* pcap */, false /*printRoutes*/,
                              37 /*totalTime */, false /*linkBreak*/,
                              "ping" /* traffic */, 0 /* numHops */,
                              0.0 /* eps */, 0.5 /* learning_rate */,
                              "test0.txt"/* test_case_filename */),
                              true, "Configuring failed for test." );

  RngSeedManager::SetSeed (1338);

  QLearningBase::DoRun();
  //
  // std::vector<std::array<int, 3> > expected_results { {3,4,2}, {6,4,0}, {3,3,2} } ;
  // for (unsigned int i = 0; i < GetQLearners().GetN() ; i++) {
  //   auto qq = dynamic_cast<QLearner&> ( *GetQLearners().Get(i) );
  //   auto aa = qq.GetAODV();
  //
  //   NS_TEST_ASSERT_MSG_EQ(aa->QStatistics(), expected_results.at(i)[0] , "Number of QPackets sent by node " << i << " did not meet our expectations.") ;
  //   // NS_TEST_ASSERT_MSG_EQ(aa->AODVStatistics(), expected_results.at(i)[1] , "Number of AODV Packets sent by node " << i << " did not meet our expectations.") ;
  //   // NS_TEST_ASSERT_MSG_EQ(aa->TrafficStatistics(), expected_results.at(i)[2] , "Number of traffic Packets sent by node " << i << " did not meet our expectations.") ;
  //   NS_TEST_ASSERT_MSG_EQ(aa->QStatistics(), qq.QStatistics(), "Number of traffic Packets counted by QLearner did not match nr counted in AODV in node " << i << ".") ;
  // }
  // std::ofstream ostream("tttt_file_out.txt");
  // ostream << Simulator::Now() << std::endl;
}

class QLrnTestSuite : public TestSuite {
public:
  QLrnTestSuite ();
};

QLrnTestSuite::QLrnTestSuite ()
  : TestSuite ("q-learner", SYSTEM)
{
  Packet::EnablePrinting();
  //Note : for these first three tests, there is always one packet dropped due to ARP cache being full
  AddTestCase (new QLearnerBasicShortTestCase, TestCase::QUICK);
  // AddTestCase (new QLearnerBasicMediumTestCase, TestCase::QUICK);
  // AddTestCase (new QLearnerBasicLongTestCase, TestCase::QUICK);
  // AddTestCase (new QLearnerFourNodesICMPTestCase, TestCase::QUICK);
  // AddTestCase (new QLearnerFiveNodesICMPTestCase, TestCase::QUICK);
  // AddTestCase (new QLearnerPINGTestCase, TestCase::QUICK);
  // AddTestCase (new QLearnerVOIPTestCase, TestCase::QUICK);
  // AddTestCase (new QLearnerVIDEOTestCase, TestCase::QUICK);
}

static QLrnTestSuite qLrnTestSuite;
