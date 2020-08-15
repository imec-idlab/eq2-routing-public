#include "qtable.h"

namespace ns3 {

/* Util */
//lengthen string

QTableEntry::QTableEntry() {
    m_my_estim = Years(10);
    m_next_hop = Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE);
    m_converged = false;
    m_learn_more = false;
    m_learn_less = false;
    m_sender_converged = false;
    m_unavailable = false;
    m_conv_threshold = 0;
    m_learn_more_threshold = 0;
    m_coeff_tally = 0.0;
    m_allow_change_to_learning = true;
    m_blacklisted = false;
    m_number_of_strikes = 0;
}

bool QTableEntry::LearnLess() {
  if (m_learn_less && m_allow_change_to_learning ) {
    m_learn_less = false;
    m_allow_change_to_learning = false;
    Simulator::Schedule (Seconds(15), MakeFunctionalEvent (
      [this] ()  {
        this->m_allow_change_to_learning = true;
      }
    ));
    // NS_LOG_UNCOND("Giving the signal to learn LESS at time " << Simulator::Now().GetSeconds() );
    return true;
  }
  else {
    return false;
  }
}

bool QTableEntry::LearnMore() {
  if (m_learn_more && m_allow_change_to_learning ) {
    m_learn_more = false;
    m_allow_change_to_learning = false;
    Simulator::Schedule (Seconds(15), MakeFunctionalEvent (
      [this] ()  {
        this->m_allow_change_to_learning = true;
      }
    ));
    // NS_LOG_UNCOND("Giving the signal to learn MORE at time " << Simulator::Now().GetSeconds() );
    return true;
  }
  else {
    return false;
  }
}

QTableEntry::QTableEntry(Ipv4Address i, Time t, float f, float ff, Ipv4Address ip) :
                                  m_next_hop(i),
                                  m_my_estim(t),
                                  m_conv_threshold(f),
                                  m_learn_more_threshold(ff),
                                  m_my_ip(ip) {
  m_real_observed_delay = 0;
  m_real_observed_loss = 0;
  m_converged = false;
  m_learn_more = false;
  m_learn_less = false;
  m_sender_converged = false;
  m_unavailable = false;
  m_coeff_tally = 0.0;
  m_allow_change_to_learning = true;
  m_blacklisted = false;
  m_number_of_strikes = 0;
}

void QTableEntry::SetSenderConverged(bool b) {
  m_sender_converged = b;
}


void QTableEntry::Unconverge() {
  m_converged = false;
}

QTableEntry::~QTableEntry() { }

void QTableEntry::SetQValue(Time t, bool _verbose) {
  std::stringstream ss;
  bool verbose = _verbose && false;
  Time old_q_value = m_my_estim;
  m_my_estim = t;
  if (verbose) { ss << Simulator::Now().GetSeconds() << " old="<< old_q_value << " new=" << t << "  n_h=" << m_next_hop << "   " ; }

  // For the calculation, one idea : https://www.calculatorsoup.com/calculators/algebra/percent-change-calculator.php
  // another :                       https://www.calculatorsoup.com/calculators/algebra/percent-difference-calculator.php
  float percent_change = std::abs(float(old_q_value.GetInteger() - t.GetInteger() ) ) / (t.GetInteger() != 0 ? t.GetInteger() : 1.0) ;
  if (percent_change < m_conv_threshold) {
    m_converged = true;
    m_learn_less = true;
    m_learn_more = false;
  } else if (m_converged && percent_change > m_conv_threshold) {
    if (verbose) {
      ss << "Converged and change larger than m_conv_threshold so Unconverging.";
    }

    Unconverge();
    if (percent_change > m_learn_more_threshold) {
      m_learn_more = true;
      m_learn_less = false;
      if (verbose) {
        ss << " Also, converged && too high for learn threshold so going to increase traffic?";
        NS_LOG_UNCOND(ss.str()) ;
      }
    }
  }

  // Dont use it for now
  // if (m_my_estim > MilliSeconds(2000)) { // TO-DO higher than 2 seconds
  //   m_unavailable = true;
  // }
}

QTable::QTable() { }

void QTable::Unconverge() {
  //rather crude method of going back to a "learning" phase. -- refined, now only unconverges those QTE's where the nexthop is marked unavailable
  for (auto dst : m_destinations) {
    for (auto& i : m_qtable[dst]) {
      if (i.HasConverged()) {
        if (!i.IsAvailable()) {
          i.Unconverge();
        }
      }
    }
  }
}

NS_LOG_COMPONENT_DEFINE ("QTable");

QTable::QTable(std::vector<Ipv4Address> _neighbours, Ipv4Address nodeip, float learning_rate, float convergence_threshold,
          float learn_more_threshold, std::vector<Ipv4Address> _unavail, std::string addition, bool _in_test, bool print_qtables, float gamma) :
  m_nodeip(nodeip),m_learningrate(learning_rate), m_convergence_threshold(convergence_threshold), m_gamma(gamma),
  m_learn_more_threshold(learn_more_threshold), m_neighbours(_neighbours), m_destinations(_neighbours),
  m_unavail(_unavail), m_in_test(_in_test), m_print_qtables(print_qtables) {

  m_qtable = std::map<Ipv4Address, std::vector<QTableEntry > >();
  for (auto neighb : m_neighbours) {
    m_qtable[neighb] = std::vector<QTableEntry >();
    for (auto i : m_neighbours) {
      if (i == neighb) {
        m_qtable[neighb].push_back(QTableEntry(i, MilliSeconds(INITIAL_QVALUE_QTABLEENTRY_VIA), m_convergence_threshold, m_learn_more_threshold, m_nodeip));
      } else { // some initial estimates i guess
        m_qtable[neighb].push_back(QTableEntry(i, MilliSeconds(INITIAL_QVALUE_QTABLEENTRY_NOT_VIA), m_convergence_threshold, m_learn_more_threshold, m_nodeip));
      }
    }
  }
  std::stringstream ss;
  ss << m_nodeip << "_qtable" << addition << ".txt";
  m_output_file_name = ss.str();

  if (!m_in_test && m_print_qtables) {
    std::ofstream ofs;
    ofs.open(m_output_file_name, std::ofstream::out | std::ofstream::trunc);
    ofs.close();
    m_out_stream = Create<OutputStreamWrapper> (m_output_file_name, std::ios::out | std::ios::ate | std::ios::app);
  }
}

void
QTable::MarkNeighbDown(Ipv4Address neighb) {
  if (std::find(m_neighbours.begin(), m_neighbours.end(), neighb ) != m_neighbours.end()) {
    if (std::find(m_unavail.begin(), m_unavail.end(), neighb) == m_unavail.end()) {
      // neighbour is not yet marked as unavail
      m_unavail.push_back(neighb);
      for (const auto& dst : m_destinations) {
        for (unsigned int i = 0; i < m_qtable[dst].size(); i++) {
          if (m_qtable[dst].at(i).GetNextHop() == neighb) {
            m_qtable[dst].at(i).SetUnavailable(true);
          }
        }
      }
      NS_LOG_DEBUG(neighb << " is down. Marked at node" << m_nodeip << "." );
    }
  } else {
    NS_LOG_DEBUG("Trying to mark a neighbour unavailable but it's not a neighbour");
  }
}

bool QTable::IsNeighbourAvailable(Ipv4Address neighb) {
  return std::find(m_unavail.begin(), m_unavail.end(), neighb) == m_unavail.end();
}

bool QTable::HasConverged(Ipv4Address dst, bool best_estim_only) {
  bool ret = true;
  QTableEntry q;
  for (const auto& i : m_qtable[dst]){
    if (!IsNeighbourAvailable(i.GetNextHop())) {
      //skip
    } else if (!best_estim_only)  {
      ret = ret && i.HasConverged();
      if (!ret) { return ret; }
    } else {
      if (q.GetQValue().GetInteger() > i.GetQValue().GetInteger()) {
        q = i;
      }
    }
  }
  // if (best_estim_only){
  //   if (q.HasConverged()) {
  //     NS_LOG_UNCOND("best estim has conv to valu of " << q.GetQValue().GetSeconds() );
  //   }
  // }
  return (ret && !best_estim_only) || (q.HasConverged() && best_estim_only);
}

void QTable::AddNeighbour( Ipv4Address neighb) {
  if (std::find(m_unavail.begin(), m_unavail.end(), neighb) != m_unavail.end()) {
    NS_LOG_ERROR("I (" << m_nodeip << ") received a SendHello message from an AODV neighbour and it was marked unavail. neighb= " << neighb << ". Unmarking it. (3x bc traffic types.)");
    auto unavail_index = std::find(m_unavail.begin(), m_unavail.end(), neighb);
    m_unavail.erase(unavail_index);
    for (const auto& dst : m_destinations) {
      for (unsigned int i = 0; i < m_qtable[dst].size(); i++) {
        if (m_qtable[dst].at(i).GetNextHop() == neighb) {
          m_qtable[dst].at(i).SetUnavailable(false);
        }
      }
    }
  } else {
    NS_LOG_ERROR("I (" << m_nodeip << ") received a SendHello message from an AODV neighbour and it was not marked unavail. neighb= " << neighb << ". (3x bc traffic types.)");
  }
  if (std::find(m_neighbours.begin(), m_neighbours.end(), neighb) == m_neighbours.end()) {
    NS_LOG_ERROR("I (" << m_nodeip << ") received a SendHello message from an AODV neighbour and it was not already a neighbour. neighb= " << neighb << ". Adding the neighbour. (3x bc traffic types.)");
    for (auto i : m_destinations) {
      if (i == neighb) {
        m_qtable[i].push_back(QTableEntry(neighb, MilliSeconds(0), m_convergence_threshold, m_learn_more_threshold, m_nodeip));
      } else { // some initial estimates i guess
        auto min_it = std::min_element(m_qtable[i].begin(), m_qtable[i].end(), [] (QTableEntry a, QTableEntry b) { return a.GetQValue() < b.GetQValue(); } );
        if (m_neighbours.size() > 0) {
          m_qtable[i].push_back(QTableEntry(neighb, min_it->GetQValue() + MilliSeconds(NEW_NEIGHBOUR_INITIAL_INCREMENT), m_convergence_threshold, m_learn_more_threshold, m_nodeip));
        } else {
          m_qtable[i].push_back(QTableEntry(neighb, MilliSeconds(NEW_NEIGHBOUR_INITIAL_INCREMENT), m_convergence_threshold, m_learn_more_threshold, m_nodeip));
        }
      }
    }
    m_neighbours.push_back(neighb);
  } else {
    NS_LOG_ERROR("I (" << m_nodeip << ") received a SendHello message from an AODV neighbour and it was already a neighbour. neighb= " << neighb << ". (3x bc traffic types.)");
  }
}

void QTable::RemoveNeighbour(Ipv4Address neighb) {
  if (std::find(m_unavail.begin(), m_unavail.end(), neighb) != m_unavail.end()) {
    NS_LOG_DEBUG("Removing unavailable neighbour " << neighb << ".");

    auto unavail_index = std::find(m_unavail.begin(), m_unavail.end(), neighb);
    auto neighbours_index = std::find(m_neighbours.begin(), m_neighbours.end(), neighb);

    NS_ASSERT_MSG(unavail_index != m_unavail.end(),"Trying to delete a neighbour that is no longer unavailable...somehow. I dont think this can ever actually happen");
    NS_ASSERT_MSG(neighbours_index != m_neighbours.end(), "Trying to delete a neighbour that has already been deleted, again, unlikely but to be sure let's check.");

    // Neighbour is still marked as unavailable -> remove it from both the list of neighbours and from the list of unavailables...
    m_unavail.erase(unavail_index);
    m_neighbours.erase(neighbours_index);
  } else {
    NS_LOG_DEBUG("Tried to remove neighbour but availability has been restored already.");
  }
}

bool fileEmpty (std::string filename) {
  std::fstream filestr;
  filestr.open(filename, std::ios::in);

  for (std::string line; std::getline(filestr, line); )
  {
    filestr.close();
    return false;
  }
  filestr.close();
  return true;
}

void
QTable::SetQValueWrapper(Ipv4Address dst, Ipv4Address next_hop, Time new_value) {
  ToFile(m_output_file_name);
  GetEntryByRef(dst,next_hop).SetQValue(new_value);
  ToFile(m_output_file_name);
}

void
QTable::ToFile(std::string filename) {
  if (m_out_stream == 0 || m_in_test || !m_print_qtables) {
    return;
  }

  if (fileEmpty(filename)) {
    for (auto i : m_destinations) {
      for (auto j : m_neighbours) {
        *(m_out_stream->GetStream ()) << i << " -> " << j << ",";
      }
    }
    *(m_out_stream->GetStream ()) << std::endl;
  }
  *(m_out_stream->GetStream ()) << Simulator::Now() << "|";
  for (auto i : m_destinations) {
    for (auto j : m_neighbours) {
      *(m_out_stream->GetStream ())  << std::find_if( m_qtable[i].begin(), m_qtable[i].end(), [j](const QTableEntry& elt){ return elt.GetNextHop() == j;} )->GetQValue() << ",";
    }
  }
  *(m_out_stream->GetStream ()) << std::endl;
}

void
QTable::FinalFile() {
  if (!m_out_stream) {
    return;
  }
  for (auto i : m_destinations) {
    for (auto j : m_neighbours) {
      *(m_out_stream->GetStream ()) << i << " >> " << j << ",";
    }
  }
  *(m_out_stream->GetStream ()) << std::endl;
}

bool
QTable::CheckDestinationKnown(const Ipv4Address& dst) {
  if (!((dst == m_nodeip) || m_qtable.find(dst) != m_qtable.end())) {
    NS_LOG_DEBUG("CheckDestinationKnown " << dst << " " << m_nodeip << "  "
                  << (std::find(m_neighbours.begin(), m_neighbours.end(), dst) != m_neighbours.end()) << "    "
                  << (std::find(m_destinations.begin(), m_destinations.end(), dst) != m_destinations.end()));
  }
  return (dst == m_nodeip) || m_qtable.find(dst) != m_qtable.end();
}

bool
QTable::AddDestination(Ipv4Address via, Ipv4Address dst, Time t) {
  NS_ASSERT_MSG(m_nodeip != dst, m_nodeip << " tried to add QRoute to itself. Let the debugging commence!");
  if (m_qtable.find(dst) != m_qtable.end()) {
    NS_LOG_DEBUG("[" << m_nodeip << "]" << "Tried adding " << dst << " via " << via << " but the destination was already known. Do we do anything instead..?");
    // dst is already known as a destination for this node, we dont have to add it again...
    return false;
  } else {
    NS_LOG_DEBUG("[" << m_nodeip << "]" << "Adding " << dst << " via " << via << "  with time value " << t << ".");
    // if (m_nodeip == Ipv4Address("10.1.1.5")) {
    //   NS_LOG_DEBUG(m_nodeip << " is adding " << dst << " as a destination with via = " << via);
    //   if (dst == Ipv4Address("10.1.1.2")){
    //     NS_ASSERT(false);
    //   }
    // }
    m_destinations.push_back(dst);
    m_qtable[dst] = std::vector<QTableEntry >();

    for (auto i : m_neighbours) {
      if (via == Ipv4Address(IP_WHEN_NO_NEXT_HOP_NEIGHBOUR_KNOWN_YET)) {
        m_qtable[dst].push_back(QTableEntry(i, MilliSeconds(INITIAL_QVALUE_QTABLEENTRY_VIA), m_convergence_threshold, m_learn_more_threshold, m_nodeip)); //this is dodgy too ...
      } else {
        if (i == via) {
          m_qtable[dst].push_back(QTableEntry(i, t,m_convergence_threshold, m_learn_more_threshold, m_nodeip));
        } else { // some initial estimates i guess
          m_qtable[dst].push_back(QTableEntry(i, t + MilliSeconds(INITIAL_QVALUE_QTABLEENTRY_NOT_VIA), m_convergence_threshold, m_learn_more_threshold, m_nodeip)); //Actually no idea...
        }
      }
    }
    return true;
  }
}

QTable::~QTable() { }

std::string lengthen_string(std::string str, int new_len) {

  return "";
}

std::string
QTable::PrettyPrint(std::string extra_info) {
  std::string tmp,tmp_C;
  std::stringstream oss,help,timeHelp;

  // Print top-left : my_ip (node) |
  m_nodeip.Print(help);
  help << "(node)";
  tmp = help.str();
  help.str(std::string());
  tmp.insert(0, 15-tmp.size(), ' ');
  oss << tmp << "   |   ";

  // Print the column headers, i.e. neighb | neighb | neighb | neighb ...
  for (const auto& dst : m_destinations) {
    for (unsigned int i = 0; i < m_qtable[dst].size(); i++) {
      m_qtable[dst].at(i).GetNextHop().Print(help);
      if (std::find(m_unavail.begin(), m_unavail.end(), m_qtable[dst].at(i).GetNextHop() ) != m_unavail.end()) {
        help << " (U)";
      }
      tmp = help.str();
      help.str(std::string());
      tmp.insert(0, 15-tmp.size(), ' ');
      oss << tmp << "    |   ";
    }
    oss << "      " << extra_info << std::endl;
    break;
  }

  // Print the rows : dst | estim | estim | estim
  for (const auto& dst : m_destinations) {
    help << dst;
    tmp = help.str();
    help.str(std::string());
    tmp.insert(0, 15-tmp.size(), ' ');

    oss << tmp << "   |   ";

    for (unsigned int i = 0; i < m_qtable[dst].size(); i++) {
      help << m_qtable[dst].at(i).GetQValue().As (Time::MS);
      tmp = help.str();
      help.str(std::string());

      if (m_qtable[dst].at(i).HasConverged() ) {
        tmp_C = "(C) ";
      } else if (m_qtable[dst].at(i).IsBlackListed() ) {
        tmp_C = "(B) ";
      } else {
        tmp_C = "";
      }
      //
      //
      std::size_t found_decimal = tmp.find(".");
      std::size_t found_ms = tmp.find("ms");

      NS_ASSERT_MSG(tmp[0] == '+', "expecting there to be a + at the start of input, otherwise undefined behaviour.");
      tmp = tmp.substr(1,found_decimal) + tmp.substr(found_decimal+1, 3);
      if (found_ms > found_decimal+4) {
        tmp += "ms";
      }

      if ((tmp_C.size() +tmp.size()) < 15) {
        tmp.insert(0, 15-(tmp.size()+tmp_C.size()), ' ');
      }
      oss << tmp_C << tmp << " " << "   |   ";

    }
    oss << std::endl;
  }
  oss << std::endl;
  return oss.str();
}

QTableEntry& QTable::GetEntryByRef(Ipv4Address dst,Ipv4Address via) {
  NS_ASSERT_MSG(std::find_if(m_qtable[dst].begin(), m_qtable[dst].end(), [via](const QTableEntry& obj) {
      return obj.GetNextHop() == via;
    } ) != m_qtable[dst].end(),
  "\nTried to find an entry by reference but it did not exist!? dst=" << dst << " via="<<via<<" and i am " << m_nodeip << std::endl << PrettyPrint() );

  return *(std::find_if(m_qtable[dst].begin(), m_qtable[dst].end(), [via](const QTableEntry& obj) {return obj.GetNextHop() == via; } ) );
}

QTableEntry
QTable::GetNextEstimToLearn(Ipv4Address dst) {
  NS_ASSERT_MSG(m_qtable.find(dst) != m_qtable.end() || dst == m_nodeip, "(best estim) If we encounter a dst we have previously not registered we will get strange behaviour. (right?)");
  QTableEntry q;
  q.SetNodeIp(m_nodeip);

  if (dst == m_nodeip) {
    return QTableEntry(q.GetNextHop(),Seconds(0), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

  for (const auto& elt : m_qtable[dst]) {
    if (  (elt.HasConverged() ) ||
          (std::find(m_unavail.begin(),m_unavail.end(),elt.GetNextHop()) != m_unavail.end())
       ) {
      //do nothing
    } else {
      q = elt;
    }
  }

  bool any_neighb_reachable = false;
  for (const auto& j : m_neighbours) {
    if ( std::find(m_unavail.begin(), m_unavail.end(), j) != m_unavail.end() ) {
      any_neighb_reachable = false || any_neighb_reachable;
      // NS_ASSERT_MSG(false, "ALL NEIGHBOURS ARE UNREACHABLE");
    } else {
      any_neighb_reachable = true;
    }
  }

  if (!any_neighb_reachable) {
    return QTableEntry(Ipv4Address(NO_NEIGHBOURS_REACHABLE_ROUTE_IP), MilliSeconds(NO_NEIGHBOURS_REACHABLE_ROUTE_MS), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

  NS_ASSERT_MSG(q.GetQValue() != Years(1), "Somehow, the estimate remained equal to Years(1) (init value) which is a problem and will mess up the sim. Location:" << m_nodeip <<  ".");
  return q;
}


QTableEntry
QTable::GetNextEstim(Ipv4Address dst, Ipv4Address via) {
  NS_ASSERT_MSG(m_qtable.find(dst) != m_qtable.end() || dst == m_nodeip, "(best estim) If we encounter a dst we have previously not registered we will get strange behaviour. (right?)");

  QTableEntry q;
  q.SetNodeIp(m_nodeip);

  if (dst == m_nodeip) {
    return QTableEntry(q.GetNextHop(),Seconds(0), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

  for (const auto& elt : m_qtable[dst]) {
    if (elt.GetNextHop() != via ||
        (elt.GetQValue() <= q.GetQValue() && std::find(m_unavail.begin(),m_unavail.end(),elt.GetNextHop()) != m_unavail.end())) {
      //do nothing
    } else {
      q = elt;
    }
  }

  bool any_neighb_reachable = false;
  for (const auto& j : m_neighbours) {
    if ( std::find(m_unavail.begin(), m_unavail.end(), j) != m_unavail.end() ) {
      any_neighb_reachable = false || any_neighb_reachable;
      // NS_ASSERT_MSG(false, "ALL NEIGHBOURS ARE UNREACHABLE");
    } else {
      any_neighb_reachable = true;
    }
  }

  if (!any_neighb_reachable) {
    return QTableEntry(Ipv4Address(NO_NEIGHBOURS_REACHABLE_ROUTE_IP), MilliSeconds(NO_NEIGHBOURS_REACHABLE_ROUTE_MS), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

  NS_ASSERT_MSG(q.GetQValue() != Years(1), "Somehow, the estimate remained equal to Years(1) (init value) which is a problem and will mess up the sim. Location:" << m_nodeip <<  ".");
  return q;
}

QTableEntry
QTable::GetNextEstim(Ipv4Address dst, Ipv4Address next_hop_a, Ipv4Address next_hop_b) {
  NS_ASSERT_MSG(m_qtable.find(dst) != m_qtable.end() || dst == m_nodeip, "(best estim) If we encounter a dst we have previously not registered we will get strange behaviour. (right?)");

  QTableEntry q;
  q.SetNodeIp(m_nodeip);

  if (dst == m_nodeip) {
    return QTableEntry(q.GetNextHop(),Seconds(0), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

  QTableEntry q_by_ref = q;
  if (q.GetNextHop() != Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE) ) {
    q_by_ref = GetEntryByRef(dst, q.GetNextHop());
  }

  for (const auto& elt : m_qtable[dst]) {
    if (elt.GetNextHop() == next_hop_a || // if its next hop a
        (elt.GetQValue() <= q.GetQValue() && std::find(m_unavail.begin(),m_unavail.end(),elt.GetNextHop()) != m_unavail.end()) ||
        elt.GetQValue() > q.GetQValue() || // or if its a worse estimate
        elt.GetNextHop() == next_hop_b || // or if its next hop b
        q_by_ref.IsBlackListed() // or if its blacklisted, BUT FOR THAT WE NEED IT BY REF
        ) {
      //do nothing
    } else {
      q = elt;
    }
  }

  if (q.GetNextHop() == Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE) && q.GetQValue() == Years(10) ) {
    NS_ASSERT(false);
    for (const auto& elt : m_qtable[dst]) {
      if (elt.GetNextHop() == next_hop_a ||
          (elt.GetQValue() > q.GetQValue() && std::find(m_unavail.begin(),m_unavail.end(),elt.GetNextHop()) != m_unavail.end()) ||
          q.IsBlackListed()
          ) {
        //do nothing
      } else {
        q = elt;
      }
    }
  }
  NS_ASSERT(!(q.GetNextHop() == Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE) && q.GetQValue() == Years(10)));

  bool any_neighb_reachable = false;
  for (const auto& j : m_neighbours) {
    if ( std::find(m_unavail.begin(), m_unavail.end(), j) != m_unavail.end() ) {
      any_neighb_reachable = false || any_neighb_reachable;
      // NS_ASSERT_MSG(false, "ALL NEIGHBOURS ARE UNREACHABLE");
    } else {
      any_neighb_reachable = true;
    }
  }

  if (!any_neighb_reachable) {
    return QTableEntry(Ipv4Address(NO_NEIGHBOURS_REACHABLE_ROUTE_IP), MilliSeconds(NO_NEIGHBOURS_REACHABLE_ROUTE_MS), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

  NS_ASSERT_MSG(q.GetQValue() != Years(1), "Somehow, the estimate remained equal to Years(1) (init value) which is a problem and will mess up the sim. Location:" << m_nodeip <<  ".");
  return q;
}


bool
QTable::AllNeighboursBlacklisted(Ipv4Address dst) {
  NS_ASSERT_MSG(dst != Ipv4Address(NO_NEIGHBOURS_REACHABLE_ROUTE_IP), "At least be a real destination IP address if you're going to check this." );
  NS_ASSERT_MSG(dst != Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE), "At least be a real destination IP address if you're going to check this." );

  bool all_neighb_blacklisted = true;
  for (const auto& j : m_neighbours) {
    if (dst == m_nodeip) {
      return false;
    } else{
      if ( std::find(m_unavail.begin(), m_unavail.end(), j) == m_unavail.end() && !GetEntryByRef(dst,j).IsBlackListed() && GetEntryByRef(dst,j).IsAvailable() ) {
        all_neighb_blacklisted = false;
      } else {
        all_neighb_blacklisted = true && all_neighb_blacklisted;
      }
    }
  }
  // if (all_neighb_blacklisted){
  //   std::cout << PrettyPrint() << std::endl;
  // }
  return all_neighb_blacklisted;
}


QTableEntry
QTable::GetNextEstim(Ipv4Address dst) {
  /* if we cant find the destination in our qtable or if the destination is still ourselves ( if we're the PTST tagged packet's destination, this case happens ) */
  NS_ASSERT_MSG(m_qtable.find(dst) != m_qtable.end() || dst == m_nodeip, "(best estim) If we encounter a dst we have previously not registered we will get strange behaviour. (right?)");

  QTableEntry q;
  q.SetNodeIp(m_nodeip);

  if (dst == m_nodeip) {
    return QTableEntry(q.GetNextHop(),Seconds(0), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

  if (AllNeighboursBlacklisted(dst) && m_nodeip == Ipv4Address("10.1.1.1") /* only the source sending randomly otherwise loops */ ) {
    // this is still experimental though
    // NS_ASSERT_MSG(false, "this part is yet to do");
    return GetRandomEstim(dst, false); //dont look at unconv only, just any random one will do..
  }

  for (const auto& elt : m_qtable[dst]) {
    if (elt.GetQValue() > q.GetQValue() || (elt.GetQValue() <= q.GetQValue() && std::find(m_unavail.begin(),m_unavail.end(),elt.GetNextHop()) != m_unavail.end())) {
      //do nothing
    } else {
      q = elt;
    }
  }

  bool any_neighb_reachable = false;
  for (const auto& j : m_neighbours) {
    if ( std::find(m_unavail.begin(), m_unavail.end(), j) != m_unavail.end() ) {
      any_neighb_reachable = false || any_neighb_reachable;
      // NS_ASSERT_MSG(false, "ALL NEIGHBOURS ARE UNREACHABLE");
    } else {
      any_neighb_reachable = true;
    }
  }

  if (!any_neighb_reachable) {
    return QTableEntry(Ipv4Address(NO_NEIGHBOURS_REACHABLE_ROUTE_IP), MilliSeconds(NO_NEIGHBOURS_REACHABLE_ROUTE_MS), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

  NS_ASSERT_MSG(q.GetQValue() != Years(1), "Somehow, the estimate remained equal to Years(1) (init value) which is a problem and will mess up the sim. Location:" << m_nodeip <<  ".");
  return q;
}

void QTableEntry::DeductStrike() {
  m_number_of_strikes -= 1;
  if (m_number_of_strikes == 0 && IsBlackListed()) {
    SetBlacklisted(false);
  }
}

void QTableEntry::AddStrike() {
  if (IsBlackListed()) {
    return;
  }
  m_number_of_strikes += 1;
  if (m_number_of_strikes == MAX_NR_STRIKES_BLACKLISTED_NODE && !IsBlackListed()) {
    SetBlacklisted(true);
  }
  NS_ASSERT_MSG(m_number_of_strikes <= MAX_NR_STRIKES_BLACKLISTED_NODE, "Shoudlnt be able to have more than 5 strikes.");
}


bool QTable::LearnLess(Ipv4Address dst) {
  /* find best estimate, if that one says to learn less & some other alternative route agrees, then learn less */
  bool ret = false;
  QTableEntry best_estim = GetNextEstim(dst);
  if (GetEntryByRef(dst,best_estim.GetNextHop()).LearnLess()) {
    for (const auto& neighb : m_neighbours) {
      if (GetEntryByRef(dst,neighb).LearnLess()) {
        ret = true;
      }
    }
  }
  // NS_LOG_UNCOND("Returning " << (ret ? "true":"false") << " in LearnLess for dst " << dst << " and best estim " << best_estim.GetNextHop());
  // NS_LOG_UNCOND(PrettyPrint());
  return ret || AllNeighboursBlacklisted(dst); //if all are blacklisted, its pointless to have a lot of learning traffic so lower it
}
bool QTable::LearnMore(Ipv4Address dst) {
  /* find best estimate, if that one says to learn more then learn more */
  bool ret = false;
  QTableEntry best_estim = GetNextEstim(dst);
  if (!AllNeighboursBlacklisted(dst)) { // if all neighbs are false, its not gonna happen!
    ret = GetEntryByRef(dst,best_estim.GetNextHop()).LearnMore();
  }
  // NS_LOG_UNCOND("Returning " << (ret ? "true":"false") << " in LearnMore for dst " << dst << " and best estim " << best_estim.GetNextHop());
  // NS_LOG_UNCOND(PrettyPrint());
  return ret;
}


QTableEntry
QTable::GetRandomEstim(Ipv4Address dst, bool _unconverged_entries_only) {
  bool unconverged_entries_only = _unconverged_entries_only;

  if (HasConverged(dst)) {
    unconverged_entries_only = false;
  }
  /* if we cant find the destination in our qtable or if the destination is still ourselves ( if we're the PTST tagged packet's destination, this case happens ) */
  NS_ASSERT_MSG(m_qtable.find(dst) != m_qtable.end() || dst == m_nodeip, "(random estim) If we encounter a dst we have previously not registered we will get strange behaviour. (right?)");

  Ipv4Address next_hop;

  if (dst == m_nodeip) {
    NS_ASSERT_MSG(false, "Ive been asked about the best route to myself, that shouldn't happen actually.");
    return QTableEntry( next_hop, Seconds(0), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

  std::set<int> tried_indices;
  int random_index = 0;
  while (true) {
    random_index = rand()%(m_neighbours.size() );
    if  (IsNeighbourAvailable(m_qtable[ dst ].at(random_index).GetNextHop() ) && //if the nieghbour isnt available -> route is no good
        ( (unconverged_entries_only && !m_qtable[ dst ].at(random_index).HasConverged()) || !unconverged_entries_only)  ){ // if were only looking for non-converged values, skip this value
     break;
    }
    tried_indices.insert(random_index);
    if (tried_indices.size() == m_neighbours.size()) {
      return QTableEntry(Ipv4Address(NO_NEIGHBOURS_REACHABLE_ROUTE_IP), MilliSeconds(NO_NEIGHBOURS_REACHABLE_ROUTE_MS), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
      NS_ASSERT_MSG(false, "ALL NEIGHBOURS ARE UNREACHABLE");
    }
  }
  return m_qtable[ dst ].at(random_index);
}

void
QTable::Update(Ipv4Address via, Ipv4Address dst, Time _queue_time_, Time travel_time, Time next_hop_estimate) {
  /* if no information is known about dst, no updates will happen and we will assert(false) so we make sure to do this first*/
  AddDestination(via, dst, travel_time);
  ToFile(m_output_file_name);
  // NS_ASSERT(false);

  Time queue_time = _queue_time_ ;

  Ipv4Address showme = Ipv4Address("10.1.1.5");
  Time old_value = Seconds(0);

  // D Qx(d,y)=a ( q + s + t - Qx(d,y) )
  Time fix_rest= Seconds(0);

  // m qtable dst is 0 for some reason

  for (auto & q : m_qtable[dst]) {
    if (via == q.GetNextHop()) {
      old_value = q.GetQValue();
      if (old_value == NanoSeconds(0)) {
        queue_time = NanoSeconds(0);
      }

      if (m_nodeip == showme){
        NS_LOG_DEBUG  ( "{" );
        NS_LOG_DEBUG  ( "  [node = " << m_nodeip << "] Received more information about a route from " << m_nodeip << " to " << dst << " via " << via << "." );
        NS_LOG_DEBUG  ( "  Old estimate was " << q.GetQValue().As (Time::MS) << "  ((( " << q.GetQValue() << ")))");
        // Given that P actually spent q units of time in x’s queue and s units in transmission to y, x can update its estimate s.t.:

        // Time new_estim = Time(m_learningrate * (MilliSeconds(1) /* q */+ MilliSeconds(1) /* s */ + t - q.second).GetInteger());
        NS_LOG_DEBUG  ( "  Trying to learn something with \n\t - queue time = " << _queue_time_.As(Time::MS) << " / if old is 0 : " << queue_time.As(Time::MS)
                  << " \n\t - travel time = " << travel_time.As(Time::MS) << " \n\t - estimate received from nextHop = " << next_hop_estimate.As(Time::MS) << " \n\t - old estimate = " << q.GetQValue().As(Time::MS));
      }


      Time new_estim = Time(m_learningrate * (queue_time /* q */+ travel_time /* s */ + next_hop_estimate - q.GetQValue()).GetInteger());
      // q.second = q.second + new_estim;

      /** Different formulas
       * Boyan's formula (paper)
       * delta_Qx(d,y)=a ( q + s + t - Qx(d,y) ) <-- but what are we supposed to actually do with the delta ?
       *
       *
       * Other formula (wikipedia)
       * Q(st, at) = (1-a) * Q(st, at) + a * ( rt + discount * estim of optimal future value)
       *
       * = new val           = old val
       *
       *
       * On QLearningTests/test2.txt, the difference in node 2 was 0.990ms vs 0.929ms and 0.881ms vs 0.899ms ...
       * For node0, (boyan's calc)
       *   10.1.1.1(node)   |          10.1.1.3   |          10.1.1.2   |
       *         10.1.1.2   |           +54.0ms   |          +3.336ms   |
       *         10.1.1.3   |          +2.416ms   |           +54.0ms   |
       *         10.1.1.4   |          +1.489ms   |        +99999.0ms   |
       *
       *
       *  vs wiki
       *   10.1.1.1(node)   |          10.1.1.3   |          10.1.1.2   |
       *         10.1.1.2   |           +54.0ms   |          +3.336ms   |
       *         10.1.1.3   |          +2.416ms   |           +54.0ms   |
       *         10.1.1.4   |          +1.489ms   |        +99999.0ms   |
       */
//      Time perceived_reward = travel_time + queue_time + Time::FromInteger(m_gamma * next_hop_estimate.GetInteger(), Time::NS);// so fix this to use the int i guess maybe ? ;
      Time perceived_reward = travel_time + queue_time + next_hop_estimate;


       // Time temp_q_for_discount_factor = Time( (1 - m_learningrate) * q.GetQValue().GetInteger()) + Time( ( m_learningrate * perceived_reward.GetInteger() ) ) ;
       // // std::cout << "new value = " << temp_q_for_discount_factor.GetSeconds();
       // QTableEntry next_state_best;
       // for (const auto& neighbour : m_neighbours) {
       //   QTableEntry tmp = GetNextEstim(dst, neighbour);
       //   if (neighbour != via && next_state_best.GetQValue() > tmp.GetQValue() ) {
       //     next_state_best = tmp;
       //   }
       // }
       // if (temp_q_for_discount_factor > next_state_best.GetQValue() && via != next_state_best.GetNextHop() ) {
       //   temp_q_for_discount_factor = next_state_best.GetQValue();
       // }
       // std::cout << " and after the gamma change thing it is = " << temp_q_for_discount_factor.GetSeconds() << "\n(nexthop = "<<via<<", and dst="<<dst<<")\n" << PrettyPrint();

       q.SetQValue(   Time( (1 - m_learningrate) * q.GetQValue().GetInteger() ) +
                      Time( ( m_learningrate * ( perceived_reward.GetInteger() + 0 *  m_gamma * next_hop_estimate.GetInteger() ) ) ) );

       q.SetCoefficientTally((1-m_learningrate) * q.GetCoefficientTally());

       fix_rest  = q.GetQValue();
       if (m_nodeip == showme) {
         NS_LOG_DEBUG ( "  Learned new estim = " << q.GetQValue().As(Time::MS) );
         NS_LOG_DEBUG ( "  Learned new estim = " << fix_rest );
         NS_LOG_DEBUG(PrettyPrint());
       }
    }
  }
  for (auto & q : m_qtable[dst]) {
    if (q.GetQValue() == MilliSeconds(INITIAL_QVALUE_QTABLEENTRY_NOT_VIA)) {
      q.SetQValue(fix_rest + MilliSeconds(INITIAL_QVALUE_QTABLEENTRY_NOT_VIA));
    }
  }
  if (m_nodeip == showme) {
    NS_LOG_DEBUG  ("After updating all values we obtain");
    NS_LOG_DEBUG  (PrettyPrint());
    NS_LOG_DEBUG  ( "}" );
  }
  ToFile(m_output_file_name);
}

uint64_t
QTable::CalculateNewQValue(Ipv4Address via, Ipv4Address dst, Time _queue_time_, Time travel_time, Time next_hop_estimate) {
  AddDestination(via, dst, travel_time);

  Time queue_time = _queue_time_ ;

  Ipv4Address showme = Ipv4Address("10.1.1.5");
  Time old_value = Seconds(0);
  Time fix_rest= Seconds(0);
  Time new_value = Seconds(99);

  for (auto & q : m_qtable[dst]) {
    if (via == q.GetNextHop()) {
      old_value = q.GetQValue();
      if (old_value == NanoSeconds(0)) {
        queue_time = NanoSeconds(0);
      }

      if (m_nodeip == showme){
        NS_LOG_DEBUG( "{ ONLY CALCULATING THE VALUE NOT UPDATING" );
        NS_LOG_DEBUG( "  [node = " << m_nodeip << "] Received more information about a route from " << m_nodeip << " to " << dst << " via " << via << "." );
        NS_LOG_DEBUG( "  Old estimate was " << q.GetQValue().As (Time::MS) << "  ((( " << q.GetQValue() << ")))");
        // Given that P actually spent q units of time in x’s queue and s units in transmission to y, x can update its estimate s.t.:

        // Time new_estim = Time(m_learningrate * (MilliSeconds(1) /* q */+ MilliSeconds(1) /* s */ + t - q.second).GetInteger());
        NS_LOG_DEBUG( "  Trying to learn something with \n\t - queue time = " << _queue_time_.As(Time::MS) << " / if old is 0 : " << queue_time.As(Time::MS)
                  << " \n\t - travel time = " << travel_time.As(Time::MS) << " \n\t - estimate received from nextHop = " << next_hop_estimate.As(Time::MS) << " \n\t - old estimate = " << q.GetQValue().As(Time::MS));
      }

      Time perceived_reward = travel_time + queue_time + next_hop_estimate;

      // Time temp_q_for_discount_factor = Time( (1 - m_learningrate) * q.GetQValue().GetInteger()) + Time( ( m_learningrate * perceived_reward.GetInteger() ) ) ;
      // QTableEntry next_state_best = GetNextEstim(dst);
      // if (temp_q_for_discount_factor > next_state_best.GetQValue() && via != next_state_best.GetNextHop() ) {
      //   temp_q_for_discount_factor = next_state_best.GetQValue();
      // }
      new_value =    Time( (1 - m_learningrate) * q.GetQValue().GetInteger()) +
                     Time( ( m_learningrate * ( perceived_reward.GetInteger() + 0 * m_gamma * next_hop_estimate.GetInteger() ) ) );

      fix_rest  = q.GetQValue();
      if (m_nodeip == showme) {
        NS_LOG_DEBUG( "  Learned new estim = " << q.GetQValue().As(Time::MS) );
        NS_LOG_DEBUG( "  Learned new estim = " << fix_rest );
        NS_LOG_DEBUG(PrettyPrint());
      }
    }
  }

 for (auto & q : m_qtable[dst]) {
   if (q.GetQValue() == MilliSeconds(INITIAL_QVALUE_QTABLEENTRY_NOT_VIA)) {
     NS_ASSERT_MSG(q.GetQValue() == MilliSeconds(1), "actually rather not have this happen.");
     q.SetQValue(fix_rest + MilliSeconds(INITIAL_QVALUE_QTABLEENTRY_NOT_VIA));
   }
 }

  if (m_nodeip == showme) {
    NS_LOG_DEBUG("After updating all values we obtain");
    NS_LOG_DEBUG(PrettyPrint());
    NS_LOG_DEBUG( "}" );
  }
  NS_ASSERT(new_value != Seconds(99));
  return new_value.GetInteger();
}


void
QTable::ChangeQValuesFromZero(Ipv4Address dst, Ipv4Address aodv_next_hop) {
  // After the mixup in commit 3f622ef where I replaced != 0ns by == 0ns I've decided to rewrite this function
  // as follows, mainly for clarity. Tests all passed at this point.
  bool all_q_equal_to_zero = true;
  for (const auto& q : m_qtable[dst]) {
    if (!(q.GetQValue() == NanoSeconds(0))) {
      all_q_equal_to_zero = false;
    }
  }
  if (all_q_equal_to_zero) {
    for (auto& q : m_qtable[dst]) {
      if (q.GetNextHop() == aodv_next_hop) {
        q.SetQValue(MilliSeconds(INITIAL_QVALUE_QTABLEENTRY_NOT_VIA));
      } else {
        q.SetQValue(MilliSeconds(INITIAL_QVALUE_QTABLEENTRY_VIA));
      }
    }
  }
  return;
}

} //namespace ns3
