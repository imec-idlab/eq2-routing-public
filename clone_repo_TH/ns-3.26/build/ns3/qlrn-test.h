#ifndef QLRNTEST_H
#define QLRNTEST_H

#include "ns3/assert.h"
#include "ns3/thomas-configuration.h"
#include "ns3/nstime.h"
#include <sstream>
#include <vector>
#include <array>
#include <iostream>
#include <string>
#include <fstream>
#include <map>
using namespace ns3;

enum EventType {
  JITTER = 0,
  DELAY  = 1,
  BREAK  = 2,
  PACKET_LOSS  = 3,
  TRAFFIC = 4,
  MOVE = 5,
};

struct TestEvent {
  int t; // time in the simulation in seconds for the event to happen
  int node_id; // node to which the change should happen
  int node_rx;
  int seconds;
  std::string traffic_type;
  EventType et;
  float new_value;
  std::string PrettyPrint() const;
};

class QLrnTest {
public:
  QLrnTest();
  ~QLrnTest();

  bool FromFile(std::string);

  std::vector<std::pair<uint32_t, uint32_t> > GetLocations() { return m_locations;}
  std::pair<uint32_t, uint32_t> GetLocationForNode(int i) { return m_locations.at(i); }
  uint32_t GetNodeDelayValue(int i) { return m_delay_per_node[i]; }
  uint32_t GetMaxY() { return m_maxY; }
  uint32_t GetMaxX() { return m_maxX; }
  uint32_t GetNoN() { return m_number_of_nodes; }
  std::string GetNameDesc() {return m_name_desc; }
  std::string GetTrafficType( ) { return m_traffictype; }
  uint32_t GetMaxPackets( ) { return m_maxpackets; }
  uint32_t GetTotalTime() { return m_totaltime; }
  float GetLearningRate() { return m_learning_rate; }
  float GetEps() { return m_eps; }
  float GetGamma() { return m_gamma; }
  bool IsLoaded() { return m_loaded; }
  bool GetIdeal() { return m_ideal; }
  std::vector<int> GetBreakIds() { return m_break_ids; }
  std::vector<int> GetQJitterIds() { return m_q_jitter_ids; }
  std::vector<int> GetL3JitterIds() { return m_l3_jitter_ids; }
  std::vector<int> GetQSlowIds() { return m_q_slow_ids; }
  std::vector<int> GetL3SlowIds() { return m_l3_slow_ids; }
  bool GetQLearningEnabled() { return m_qlearn_enabled; }
  bool GetLearningPhases() { return m_learning_phases; }
  bool GetVaryingBreakTimes() { return m_vary_break_times; }
  bool GetQoSQLearning() { return m_qos_qlearning; }
  bool GetMetricsBackToSrcEnabled() { return m_metrics_back_to_src; }
  bool GetSmallLearningTraffic() { return m_small_learning_stream; }
  float GetQConvThresh() { return m_q_conv_thresh; }
  float GetRho() { return m_rho; }
  int GetMaxRetry() { return m_max_retry; }
  float GetLrnMoreThreshold() { return m_learn_more_threshold; }
  std::string PrettyPrint();
  std::vector<TestEvent> GetEvents() { return m_test_events; }
private:
  bool ParseNode(uint32_t index, const std::vector<std::string>& file, int&);
  bool ParseEvent(uint32_t index, const std::vector<std::string>& file);
  std::vector<std::pair<uint32_t, uint32_t> > m_locations;

  bool m_loaded;

  uint32_t m_number_of_nodes;
  std::string m_name_desc;
  std::string m_traffictype;
  uint32_t m_totaltime;
  float m_eps;
  float m_learning_rate;
  bool m_ideal;
  bool m_qos_qlearning;
  bool m_metrics_back_to_src;
  uint32_t m_maxpackets;
  bool m_learning_phases;
  std::vector<int> m_break_ids;
  std::vector<int> m_q_jitter_ids;
  std::vector<int> m_l3_jitter_ids;
  std::vector<int> m_q_slow_ids;
  std::vector<int> m_l3_slow_ids;
  std::map<int,uint32_t> m_delay_per_node;
  bool m_qlearn_enabled;
  bool m_vary_break_times;
  bool m_small_learning_stream;
  uint32_t m_maxX;
  uint32_t m_maxY;
  float m_rho;
  float m_q_conv_thresh;
  int m_max_retry;
  float m_gamma;
  float m_learn_more_threshold;
  std::vector<TestEvent> m_test_events;
};

class QLrnTestResult {
public:
  QLrnTestResult();
  ~QLrnTestResult();

  bool FromFile(std::string);
  std::string Print();
  uint32_t GetNodeTrafficExpectedResult(int,int);
  std::vector<int> GetExpectedQRoute() { return m_expected_qroute; } // return qroute to check if QoS traffic is using the "good" route
  std::vector<int> GetExpectedBroken() { return m_expected_broken; }
  std::vector<int> GetExpectedLearning() { return m_expected_learning; }
  std::vector<int> GetExpectedNotLearning() { return m_expected_not_learning; }
  int GetLowerBoundDelay() { return m_avg_delay_in_ms_lowerbound != VALUE_UNUSED_INT_PARAMETER? m_avg_delay_in_ms_lowerbound * 1000000 : m_avg_delay_in_ms_lowerbound; } //from ms to NS for the simulator so multiply && add the if in case its value int unused param for int overflow
  uint32_t GetNoN() { return m_expected_results.size(); }
  bool     IsLoaded() { return m_loaded; }
  void     ParseNode(uint32_t index, const std::vector<std::string>& delims, uint32_t& q, uint32_t& a, uint32_t& t, uint32_t& t_rec, uint32_t& t_lrn,uint32_t& t_lrn_rec, bool& broken, uint32_t& learning, int& nr_var);
private:
  bool m_loaded;

  //
  std::vector<int> m_expected_qroute;
  std::vector<int> m_expected_broken;
  std::vector<int> m_expected_learning;
  std::vector<int> m_expected_not_learning;
  //
  int m_avg_delay_in_ms_lowerbound;
  /// node is the index for the vector, 1st value in array is Q, 2nd is AODV, 3rd is traffic...
  std::vector<std::array<uint32_t, 6> > m_expected_results;

};

#endif /* QLRNTEST_H */
