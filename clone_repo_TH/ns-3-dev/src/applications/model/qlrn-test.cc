#include "qlrn-test.h"

// Currently no error handling present..

std::string
TestEvent::PrettyPrint() const {
  std::stringstream ss;
  std::string event_type_str;
  if (et == JITTER) { event_type_str  = "JITTER"; }
  else if (et == DELAY) { event_type_str = "DELAY"; }
  else if (et == BREAK) { event_type_str = "BREAK"; }
  else if (et == PACKET_LOSS) { event_type_str = "PACKET_LOSS"; }
  else if (et == TRAFFIC) { event_type_str = "TRAFFIC"; }
  else if (et == MOVE) { event_type_str = "MOVE"; }
  else { NS_FATAL_ERROR("couldnt parse event type"<<et); }

  if (et != TRAFFIC) {
    ss << "An event scheduled at " << t << "s to set " << event_type_str << " to " << new_value << " on node " << node_id << std::endl;
  } else {
    ss  << "An event scheduled at " << t << "s to set " << event_type_str << " to " << new_value << " of type " << traffic_type
        << " from node " << node_id << " to node " << node_rx << "  for " << seconds << " seconds." << std::endl;
  }
  return ss.str();
}


QLrnTest::QLrnTest() : m_loaded(false), m_name_desc("NOT INITIALIZED!"), m_maxX(0), m_maxY(0) { }

QLrnTest::~QLrnTest() { }

std::string GetValue(std::string delim, std::string info_from_test, int& nr_of_variables, std::string default_value = "0") {
  uint begin, end;

  auto i = info_from_test.find(delim);
  auto j = info_from_test.find(",");

  if (i == std::string::npos) {
    begin = VALUE_UNUSED_INT_PARAMETER;
  } else {
    begin = i + delim.size();
  }

  if (j == std::string::npos) {
    end = VALUE_UNUSED_INT_PARAMETER;
  } else {
    end = j - begin;
  }

  if (begin != VALUE_UNUSED_INT_PARAMETER && end != VALUE_UNUSED_INT_PARAMETER && end != 0) {
    nr_of_variables++;
    return info_from_test.substr(begin , end);
  } else if (end == 0) {
    NS_FATAL_ERROR("Missing value in test result.");
  }
  return default_value;
}

bool QLrnTest::ParseEvent(uint32_t ii, const std::vector<std::string>& file) {
  uint curr_index = ii;

  uint begin, end;
  size_t i, j;
  std::string delim = "\"event\":{";

  while (true) {
    i = file[curr_index].find(delim);
    j = file[curr_index].find("},");
    if (i == std::string::npos) { begin = VALUE_UNUSED_INT_PARAMETER; }
    else { begin = i + delim.size(); }
    if (j == std::string::npos) { end = VALUE_UNUSED_INT_PARAMETER; }
    else { end = j - begin; }

    if (end == VALUE_UNUSED_INT_PARAMETER || begin == VALUE_UNUSED_INT_PARAMETER) { break; }

    TestEvent t_event;
    std::string parse_tmp;
    std::stringstream string_tmp(file[curr_index].substr(begin , end));
    std::getline(string_tmp, parse_tmp, '-');
    t_event.t = std::stoi(parse_tmp);
    std::getline(string_tmp, parse_tmp, '-');
    t_event.node_id = std::stoi(parse_tmp);;
    std::getline(string_tmp, parse_tmp, '-');
    if (parse_tmp == "JITTER") { t_event.et  = JITTER; }
    else if (parse_tmp == "DELAY") { t_event.et = DELAY; }
    else if (parse_tmp == "BREAK") { t_event.et = BREAK; }
    else if (parse_tmp == "PACKET_LOSS") { t_event.et = PACKET_LOSS; }
    else if (parse_tmp == "TRAFFIC") { t_event.et = TRAFFIC; }
    else if (parse_tmp == "MOVE") { t_event.et = MOVE; }
    else { NS_FATAL_ERROR("couldnt parse event type"<<parse_tmp); }
    std::getline(string_tmp, parse_tmp, '-');
    t_event.new_value = std::stof(parse_tmp);

    if (t_event.et == TRAFFIC) {
      std::getline(string_tmp, parse_tmp, '-');
      t_event.node_rx = std::stoi(parse_tmp);
      NS_ASSERT_MSG(t_event.new_value != t_event.node_rx, "Probably forgot a value bae.");
      std::getline(string_tmp, parse_tmp, '-');
      t_event.seconds = std::stoi(parse_tmp);
      NS_ASSERT_MSG(t_event.seconds != t_event.node_rx, "Probably forgot a value bae.");
      std::getline(string_tmp, parse_tmp, '-');
      t_event.traffic_type = parse_tmp;
    }
    m_test_events.push_back(t_event);

    std::cout << t_event.PrettyPrint() ;
    curr_index += 1;
  }

  return false;


}

bool QLrnTest::ParseNode(uint32_t i, const std::vector<std::string>& file, int& nr_of_variables) {
  uint32_t x = std::stoi(GetValue("\"x\":", file[i+nr_of_variables], nr_of_variables, VALUE_UNUSED_INT_PARAMETER_STR));
  uint32_t y = std::stoi(GetValue("\"y\":", file[i+nr_of_variables], nr_of_variables, VALUE_UNUSED_INT_PARAMETER_STR));
  std::string break_val   = GetValue("\"break\":", file[i+nr_of_variables], nr_of_variables, "n");
  std::string jitter_val  = GetValue("\"jitter\":", file[i+nr_of_variables], nr_of_variables, "n");
  std::string slow_val    = GetValue("\"slower\":", file[i+nr_of_variables], nr_of_variables, "n");
  uint32_t delay_value = std::stoi(GetValue("\"delay_value\":", file[i+nr_of_variables], nr_of_variables, VALUE_UNUSED_INT_PARAMETER_STR));

  if (x == VALUE_UNUSED_INT_PARAMETER || y == VALUE_UNUSED_INT_PARAMETER
      || (break_val != "y" && break_val != "n")
      || (jitter_val != "Q" && jitter_val != "l3" && jitter_val != "n")
      || (slow_val != "Q" && slow_val != "l3" && slow_val != "n") ) {
    return false;
  }

  if (break_val  == "y") {  m_break_ids.push_back( m_locations.size() ) ; }

  if (jitter_val == "Q") { m_q_jitter_ids.push_back (m_locations.size() ) ; }
  else if (jitter_val == "l3") { m_l3_jitter_ids.push_back (m_locations.size() ) ; }

  if (slow_val   == "Q") {   m_q_slow_ids.push_back( m_locations.size() ) ; }
  else if (slow_val   == "l3") {   m_l3_slow_ids.push_back( m_locations.size() ) ; }

  if ((slow_val != "n" || jitter_val != "n") && delay_value != VALUE_UNUSED_INT_PARAMETER) { m_delay_per_node[m_locations.size()] = delay_value; }

  m_locations.push_back(std::make_pair(x,y));

  return true;
}

bool QLrnTest::FromFile(std::string filename) {
  std::ifstream infile(filename);
  std::string line;
  std::vector<std::string> file;

  if (!infile.good()) {
    std::cerr << "test file " << filename << " does not seem to exist.. \n";
    return false;
  }
  try {
    while (std::getline(infile,line)){
      file.push_back(line);
    }
  } catch (std::exception & e) {
    std::cout << "Got an exception while trying to read the file." << e.what() << std::endl;
  }

  uint8_t i = 0;

  int nr_of_variables_node = 1, running_index = 0;
  if (file[i].find("{") != std::string::npos) {
    int nr_of_variables = 1;

    m_name_desc = GetValue("\"NameDesc\":", file[i+nr_of_variables], nr_of_variables);
    m_number_of_nodes = std::stoi(GetValue("\"Number of nodes\":", file[i+nr_of_variables], nr_of_variables));
    m_traffictype = GetValue("\"trafficType\":",file[i+nr_of_variables], nr_of_variables) ;
    m_totaltime = std::stoi(GetValue("\"totalTime\":",file[i+nr_of_variables], nr_of_variables) );
    m_eps = std::stof( GetValue("\"eps\":",file[i+nr_of_variables], nr_of_variables) );
    m_learning_rate =  std::stof( GetValue("\"learning_rate\":",file[i+nr_of_variables], nr_of_variables) );
    m_ideal = (GetValue("\"ideal\":",file[i+nr_of_variables], nr_of_variables)  != "false");
    m_maxpackets = std::stoi(GetValue("\"maxpackets\":",file[i+nr_of_variables], nr_of_variables ) );
    m_learning_phases = (GetValue("\"learning_phases\":",file[i+nr_of_variables], nr_of_variables)  == "true");
    m_qlearn_enabled = (GetValue("\"qlearn\":",file[i+nr_of_variables], nr_of_variables)  != "false");
    m_vary_break_times = (GetValue("\"vary_break_times\":",file[i+nr_of_variables], nr_of_variables)  == "true");
    m_qos_qlearning = (GetValue("\"qos_qlearning\":",file[i+nr_of_variables], nr_of_variables)  == "true");
    m_metrics_back_to_src = (GetValue("\"metrics_back_to_src\":",file[i+nr_of_variables], nr_of_variables)  == "true");
    m_small_learning_stream = (GetValue("\"smaller_learning_traffic\":",file[i+nr_of_variables], nr_of_variables)  == "true");
    m_rho = std::stof(GetValue("\"rho\":",file[i+nr_of_variables], nr_of_variables, "0.99"));
    m_q_conv_thresh = std::stof(GetValue("\"qconv\":",file[i+nr_of_variables], nr_of_variables, "0.025"));
    m_max_retry = std::stoi(GetValue("\"max_retry\":",file[i+nr_of_variables], nr_of_variables, "4"));
    m_learn_more_threshold = std::stof(GetValue("\"learn_more_threshold\":",file[i+nr_of_variables], nr_of_variables, "0.025"));
    m_gamma = std::stof( GetValue("\"gamma\":",file[i+nr_of_variables], nr_of_variables,"0") );

    running_index = i+nr_of_variables+1; //+1 to go to the nodeX{ line
    for (uint j = 0; j < m_number_of_nodes; j++) {
      nr_of_variables_node = 0;
      if (!ParseNode(running_index, file, nr_of_variables_node)) {
        return false;
      }
      running_index += nr_of_variables_node + 2;
    }
    ParseEvent(running_index-1, file);
  } else {
    return false;
  }

  m_loaded = true;
  return true;
}

std::string QLrnTest::PrettyPrint() {
  std::stringstream ss;
  ss << "===  " << m_name_desc << " ===" << std::endl << std::endl;
  ss << "Number of nodes: " << m_number_of_nodes  << "     - TrafficType:  "      << m_traffictype << "     - TotalTime:  " << m_totaltime << std::endl;
  ss << "m_eps:           " << m_eps              << "     - m_learning_rate:  "  << m_learning_rate << (m_ideal ? " - ideal: y":"") << (m_qos_qlearning ? "     - QosQ: y":"") << (m_learning_phases ? "      - using learning ph: y":" no learning phases" )<< std::endl;
  ss << "m_maxpackets:    " << m_maxpackets;

  ss << (m_break_ids.size() > 0 ? "     - Break Ids: " : "");
  for (const auto& i : m_break_ids) { ss << i << ",";} ss << (m_q_jitter_ids.size() > 0 ? "     - QJitter:  " : "");
  for (const auto& i : m_q_jitter_ids) { ss << i << ",";} ss << (m_l3_jitter_ids.size() > 0 ? "     - L3Jitter: " : "");
  for (const auto& i : m_l3_jitter_ids) { ss << i << ",";} ss << (m_q_slow_ids.size() > 0 ? "     - QSlow:  " : "");
  for (const auto& i : m_q_slow_ids) { ss << i << ",";} ss << (m_l3_slow_ids.size() > 0 ? "     - L3SLow: " : "");
  for (const auto& i : m_l3_slow_ids) { ss << i << ",";} ss << "     - " << (m_qlearn_enabled ? "qlearn enabled: Y" : "qlearn enabled: N" );
  ss << (m_vary_break_times ? "     - Varied break times":"     - Nodes break at once");
  ss << (m_metrics_back_to_src ? "     - Metrics about real time are being comm'ed back to src":"     - Metrics about real time are not being sent back to src") << std::endl;
  ss << (m_small_learning_stream ? "    - A small stream of learning traffic remains in data phase" : " no small learning traffic remains." ) << std::endl;
  ss << "Rho = " << m_rho << "     - max retry = " << m_max_retry << "     - m_qconv = " << m_q_conv_thresh << "     - m_lrn_more_th = " << m_learn_more_threshold << "    - gamma = " << m_gamma;
  ss <<  std::endl << std::endl;
  return ss.str();
}
/////////////////////////////////////////////////////////QLRNTESTRESULTS ///////
QLrnTestResult::QLrnTestResult() : m_loaded(false) { }

QLrnTestResult::~QLrnTestResult() { }

uint32_t QLrnTestResult::GetNodeTrafficExpectedResult(int nodeID, int type) {
  // type = 0 --> QTraffic
  // type = 1 --> AODV traffic
  // type = 2 --> regular traffic
  return m_expected_results.at(nodeID)[type];
}

void CompareQRoute() {
  //pass
}

void QLrnTestResult::ParseNode(uint32_t index, const std::vector<std::string>& file, uint32_t& q, uint32_t& a, uint32_t& t,
                               uint32_t& t_rec, uint32_t& t_lrn, uint32_t& t_lrn_rec, bool& broken, uint32_t& learning, int& nr_of_variables) {
  int old_value;
  while (file[index+nr_of_variables].find("}") == std::string::npos) {
    old_value = nr_of_variables;
    if (q ==VALUE_UNUSED_INT_PARAMETER) q = std::stoi(GetValue("\"q\":", file[index+nr_of_variables], nr_of_variables, VALUE_UNUSED_INT_PARAMETER_STR));
    if (a ==VALUE_UNUSED_INT_PARAMETER) a = std::stoi(GetValue("\"a\":", file[index+nr_of_variables], nr_of_variables, VALUE_UNUSED_INT_PARAMETER_STR));
    if (t ==VALUE_UNUSED_INT_PARAMETER) t = std::stoi(GetValue("\"t\":", file[index+nr_of_variables], nr_of_variables, VALUE_UNUSED_INT_PARAMETER_STR));
    if (!broken) broken = (GetValue("\"broken\":", file[index+nr_of_variables], nr_of_variables) == "y");
    if (t_rec ==VALUE_UNUSED_INT_PARAMETER) t_rec = std::stoi(GetValue("\"t_rec\":", file[index+nr_of_variables], nr_of_variables, VALUE_UNUSED_INT_PARAMETER_STR));
    if (t_lrn_rec ==VALUE_UNUSED_INT_PARAMETER) t_lrn_rec = std::stoi(GetValue("\"t_lrn_rec\":", file[index+nr_of_variables], nr_of_variables, VALUE_UNUSED_INT_PARAMETER_STR));
    if (t_lrn ==VALUE_UNUSED_INT_PARAMETER) t_lrn = std::stoi(GetValue("\"t_lrn\":", file[index+nr_of_variables], nr_of_variables, VALUE_UNUSED_INT_PARAMETER_STR));
    if (learning==VALUE_UNUSED_INT_PARAMETER) {
      std::string learning_value = GetValue("\"learning\":", file[index+nr_of_variables], nr_of_variables);
      if (learning_value == "y"){
        learning = BOOL_PARAM_SET_TRUE; //used
      } else if (learning_value == "n"){
        learning = BOOL_PARAM_SET_FALSE;
      }
    }

    NS_ASSERT_MSG(nr_of_variables != old_value, "Infinite loop in test result parsing. Error in test file or in test script. " << file[index+nr_of_variables]);

  }
}

// thanks https://stackoverflow.com/questions/236129/the-most-elegant-way-to-iterate-the-words-of-a-string
std::vector<int> split_q_route_input(const std::string &s, char delim) {
    std::vector<int> elems;
    auto result = std::back_inserter(elems);
    std::stringstream ss(s);
    std::string item;

    while (std::getline(ss, item, delim)) {
      if (item == "*")  { *(result++) = Q_ROUTE_WILDCARD_VALUE; } //for situations where it comes down to a random result
      else              { *(result++) = std::stoi(item); } //needlessly complex
    }

    return elems;
}

bool QLrnTestResult::FromFile(std::string filename) {
  std::ifstream infile;

  try {
    infile.open(filename);
  } catch (std::exception & e) {
    std::cout << e.what() << std::endl;
    NS_ASSERT(false);
  }

  std::string line;
  std::vector<std::string> file;
  int nr_of_variables = 0, qroute_variable = 0, delay_variable = 0, tmp = 5;

  while (std::getline(infile,line)){
    file.push_back(line);
  }
  uint32_t i = 0, nodeid = 0;
  if (file[0].find("{") != std::string::npos) {   //begin

    std::string qroute = "unused";
    m_avg_delay_in_ms_lowerbound = 5;

    while (tmp != qroute_variable + delay_variable) {
      tmp = qroute_variable + delay_variable;
      if (qroute == "unused" || qroute == VALUE_UNUSED_INT_PARAMETER_STR) {
        qroute = GetValue("\"qroute\":", file[i+1+delay_variable], qroute_variable, VALUE_UNUSED_INT_PARAMETER_STR);
      }
      if (m_avg_delay_in_ms_lowerbound == 5 || m_avg_delay_in_ms_lowerbound == VALUE_UNUSED_INT_PARAMETER) {
        m_avg_delay_in_ms_lowerbound = std::stoi(GetValue("\"observed_avg_delay_ms\":", file[i+1+qroute_variable], delay_variable, VALUE_UNUSED_INT_PARAMETER_STR));
      }
    }
    if (qroute != VALUE_UNUSED_INT_PARAMETER_STR) {  m_expected_qroute = split_q_route_input(qroute, '-');  }

    while (true) {
      NS_ASSERT(file[i+delay_variable+qroute_variable+1].find("node") != std::string::npos); //ensure a node is in the 1st line after first { // -- added qroute_variable to add 1 line if a qroute is there

      //begin parsing of node (setting variables)
      nr_of_variables = 0;
      uint32_t q=VALUE_UNUSED_INT_PARAMETER,a=VALUE_UNUSED_INT_PARAMETER,t=VALUE_UNUSED_INT_PARAMETER,t_rec=VALUE_UNUSED_INT_PARAMETER,t_lrn=VALUE_UNUSED_INT_PARAMETER;
      uint32_t t_lrn_rec=VALUE_UNUSED_INT_PARAMETER,learning=VALUE_UNUSED_INT_PARAMETER;
      bool broken=false;

      // Skip first two lines, as they are not variable lines...
      // since nr_of_variables = 0, we go to the line with the first variable to start parsing the node's variables
      // then when we're done, we have nr_of_variables, which is the value we have to add to i at some point
      ParseNode(i+2+qroute_variable+delay_variable, file, q, a, t, t_rec, t_lrn, t_lrn_rec, broken, learning, nr_of_variables);

      m_expected_results.push_back(std::array<uint32_t,6>{q,a,t,t_rec,t_lrn, t_lrn_rec});

      if (broken) {  m_expected_broken.push_back(nodeid); }
      if (learning == BOOL_PARAM_SET_TRUE ) { m_expected_learning.push_back(nodeid); }
      if (learning == BOOL_PARAM_SET_FALSE ) { m_expected_not_learning.push_back(nodeid);}

      if (i > 9000){
        NS_ASSERT(false);

      }
      NS_ASSERT(file[i+2+nr_of_variables + qroute_variable + delay_variable].find("}") != std::string::npos); //i+2 same as before
      i += 2 + nr_of_variables + qroute_variable + delay_variable; //do the actual adding up for the following node (end at },  then the next node does +2 to jump over nodei again etc
      if (file[i+1].find("}") != std::string::npos) {
        break;
      }
      qroute_variable = 0;
      delay_variable = 0;
      nodeid++;
    }
  }
  m_loaded = true;
  return true;
}

std::string QLrnTestResult::Print() {
  std::stringstream ss;
  for (unsigned int i = 0; i < m_expected_results.size() ; i++) {
    ss << "For node" << i << "  " << m_expected_results[i][0] << "(Q) " << m_expected_results[i][1] << "(A) " << m_expected_results[i][2] << "(T)" << std::endl;
  }
  return ss.str();
}
