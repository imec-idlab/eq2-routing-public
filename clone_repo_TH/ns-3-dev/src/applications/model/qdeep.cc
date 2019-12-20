#include "qdeep.h"

namespace ns3 {

/* Util */
//lengthen string

QDeepEntry::QDeepEntry() {
//	  HANS - probably not needed, but unsure
//    m_my_estim = Years(10);
//    m_next_hop = Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE);
//    m_converged = false;
//    m_learn_more = false;
//    m_learn_less = false;
//    m_sender_converged = false;
//    m_unavailable = false;
//    m_conv_threshold = 0;
//    m_learn_more_threshold = 0;
//    m_coeff_tally = 0.0;
//    m_allow_change_to_learning = true;
//    m_blacklisted = false;
//    m_number_of_strikes = 0;
}

bool QDeepEntry::LearnLess() {
	// Always return false, because the learning rate is handled within the NN
    return false;
}

bool QDeepEntry::LearnMore() {
	// Always return false, because the learning rate is handled within the NN
    return false;
}

// TODO HANS - What is needed to create a new QDeepEntry? (aka, the return type of getNextEstim)
QDeepEntry::QDeepEntry(Ipv4Address i, Time t, float f, float ff, Ipv4Address ip) :
                                 QDecisionEntry(i, t, f, ff, ip) {
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

// TODO HANS - Is it OK to just ignore this in QDeep?
void QDeepEntry::SetSenderConverged(bool b) {
  m_sender_converged = b;
}


// TODO HANS - Is it OK to just ignore this in QDeep?
void QDeepEntry::Unconverge() {
  m_converged = false;
}

QDeepEntry::~QDeepEntry() { }


// TODO HANS - SetQValue --> when is this called? --> IN QosQLearner and internally
void QDeepEntry::SetQValue(Time t, bool _verbose) {
//	TODO HANS - I put the entire SetQValue function in comments. We will not set a QValue when using Deep learning

//  std::stringstream ss;
//  bool verbose = _verbose && false;
//  Time old_q_value = m_my_estim;
//  m_my_estim = t;
//  if (verbose) { ss << Simulator::Now().GetSeconds() << " old="<< old_q_value << " new=" << t << "  n_h=" << m_next_hop << "   " ; }
//
//  // For the calculation, one idea : https://www.calculatorsoup.com/calculators/algebra/percent-change-calculator.php
//  // another :                       https://www.calculatorsoup.com/calculators/algebra/percent-difference-calculator.php
//  float percent_change = std::abs(float(old_q_value.GetInteger() - t.GetInteger() ) ) / (t.GetInteger() != 0 ? t.GetInteger() : 1.0) ;
//  if (percent_change < m_conv_threshold) {
//    m_converged = true;
//    m_learn_less = true;
//    m_learn_more = false;
//  } else if (m_converged && percent_change > m_conv_threshold) {
//    if (verbose) {
//      ss << "Converged and change larger than m_conv_threshold so Unconverging.";
//    }
//
//    Unconverge();
//    if (percent_change > m_learn_more_threshold) {
//      m_learn_more = true;
//      m_learn_less = false;
//      if (verbose) {
//        ss << " Also, converged && too high for learn threshold so going to increase traffic?";
//        NS_LOG_UNCOND(ss.str()) ;
//      }
//    }
//  }

  // Dont use it for now
  // if (m_my_estim > MilliSeconds(2000)) { // TO-DO higher than 2 seconds
  //   m_unavailable = true;
  // }
}

QDeep::QDeep() : QDecision(){}

void QDeep::Unconverge() {
//	TODO HANS - I deleted the unconverge function. I assume backpropagation will update unhealthy nodes
}

NS_LOG_COMPONENT_DEFINE ("QDeep");

QDeep::QDeep(std::vector<Ipv4Address> _neighbours, Ipv4Address nodeip, float learning_rate, float convergence_threshold,
		float learn_more_threshold, std::vector<Ipv4Address> _unavail, std::string addition, bool _in_test, bool print_qtables, float gamma) :
		QDecision(_neighbours, nodeip, learning_rate, convergence_threshold, learn_more_threshold, _unavail, addition, _in_test,print_qtables, gamma){

	// TODO HANS - Most important one - decide on the number of layers and inputs
	gnn = genann_init(2, 1, 2, 1);

//	TODO HANS - instead of filling m_table up with neighbours, initialize the NN

//  for (auto neighb : m_neighbours) {
//    m_qtable[neighb] = std::vector<QDecisionEntry *>();
//    for (auto i : m_neighbours) {
//      if (i == neighb) {
//        m_qtable[neighb].push_back(new QDeepEntry(i, MilliSeconds(INITIAL_QVALUE_QTABLEENTRY_VIA), m_convergence_threshold, m_learn_more_threshold, m_nodeip));
//      } else { // some initial estimates i guess
//        m_qtable[neighb].push_back(new QDeepEntry(i, MilliSeconds(INITIAL_QVALUE_QTABLEENTRY_NOT_VIA), m_convergence_threshold, m_learn_more_threshold, m_nodeip));
//      }
//    }
//  }

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

// TODO HANS - For now we assume we don't need to know when the neighbours are down
void
QDeep::MarkNeighbDown(Ipv4Address neighb) {
  if (std::find(m_neighbours.begin(), m_neighbours.end(), neighb ) != m_neighbours.end()) {
    if (std::find(m_unavail.begin(), m_unavail.end(), neighb) == m_unavail.end()) {
      // neighbour is not yet marked as unavail
      m_unavail.push_back(neighb);
      // TODO HANS - Figure out if we have to also set this neighbour to unavailable on the other nodes
//      for (const auto& dst : m_destinations) {
//        for (unsigned int i = 0; i < m_qtable[dst].size(); i++) {
//          if (m_qtable[dst].at(i)->GetNextHop() == neighb) {
//            m_qtable[dst].at(i)->SetUnavailable(true);
//          }
//        }
//      }
      NS_LOG_DEBUG(neighb << " is down. Marked at node" << m_nodeip << "." );
    }
    // if the neighbour is already unavailable, than do nothing
  } else {
    NS_LOG_DEBUG("Trying to mark a neighbour unavailable but it's not a neighbour");
  }
}

// 	TODO HANS - For now we assume al neighbours are available
bool QDeep::IsNeighbourAvailable(Ipv4Address neighb) {
  return std::find(m_unavail.begin(), m_unavail.end(), neighb) == m_unavail.end();
   // loop through all unavailable nodes and if neighbour is not in it, it is available
}

//	TODO HANS - Can we just assume that the q network has always converged?
bool QDeep::HasConverged(Ipv4Address dst, bool best_estim_only) {
	return true;
//  bool ret = true;
//  QDecisionEntry *q = new QDeepEntry();
//  for (const auto& i : m_qtable[dst]){
//    if (!IsNeighbourAvailable(i->GetNextHop())) {
//      //skip if the current neighbour is not available
//    } else if (!best_estim_only)  {
//      ret = ret && i->HasConverged();
//      // as soon as a neighbour has not converged, we know the qtable is not fully converged
//      if (!ret) {
//    	  return ret;
//      }
//    } else {
//    	if (q->GetQValue().GetInteger() > i->GetQValue().GetInteger()) {
//        	q = i;
//    	}
//    }
//  }

// There was an additional part here that was already commented out before.
// So I deleted it, in case I uncommented everything, this would have broken it.
// If it is needed, do check how it is implemented in qtable.cc

//  return (ret && !best_estim_only) || (q->HasConverged() && best_estim_only);
}

// 	TODO HANS - Do we need to keep track of neighbours?
void QDeep::AddNeighbour( Ipv4Address neighb) {
	// if neighbour IS unavailable ...
  if (std::find(m_unavail.begin(), m_unavail.end(), neighb) != m_unavail.end()) {
    NS_LOG_ERROR("I (" << m_nodeip << ") received a SendHello message from an AODV neighbour and it was marked unavail. neighb= " << neighb << ". Unmarking it. (3x bc traffic types.)");
    auto unavail_index = std::find(m_unavail.begin(), m_unavail.end(), neighb);
    // the remove the neighbour from unavailable ...
    m_unavail.erase(unavail_index);
//	  TODO HANS - Figure out if it is needed to inform the neighbours about this.
//    for (const auto dst : m_destinations) {
//      for (unsigned int i = 0; i < m_qtable[dst].size(); i++) {
//        if (m_qtable[dst].at(i)->GetNextHop() == neighb) {
        // and for this destination, let it know that the neighbour is now available
//          m_qtable[dst].at(i)->SetUnavailable(false);
//        }
//      }
//    }
  } else {
    NS_LOG_ERROR("I (" << m_nodeip << ") received a SendHello message from an AODV neighbour and it was not marked unavail. neighb= " << neighb << ". (3x bc traffic types.)");
  }
  // If the new neighbour IS NOT already a neighbour
  if (std::find(m_neighbours.begin(), m_neighbours.end(), neighb) == m_neighbours.end()) {
    NS_LOG_ERROR("I (" << m_nodeip << ") received a SendHello message from an AODV neighbour and it was not already a neighbour. neighb= " << neighb << ". Adding the neighbour. (3x bc traffic types.)");
    	// HANS - no need to initialize new paths
    //    for (auto i : m_destinations) {
//      if (i == neighb) {
//        m_qtable[i].push_back(new QDeepEntry(neighb, MilliSeconds(0), m_convergence_threshold, m_learn_more_threshold, m_nodeip));
//      } else { // some initial estimates i guess
//    	  auto min_it = *std::min_element(m_qtable[i].begin(), m_qtable[i].end(), [] (QDecisionEntry* a, QDecisionEntry* b) { return a->GetQValue() < b->GetQValue(); } );
//        if (m_neighbours.size() > 0) {
//          m_qtable[i].push_back(new QDeepEntry(neighb, min_it->GetQValue() + MilliSeconds(NEW_NEIGHBOUR_INITIAL_INCREMENT), m_convergence_threshold, m_learn_more_threshold, m_nodeip));
//        } else {
//          m_qtable[i].push_back(new QDeepEntry(neighb, MilliSeconds(NEW_NEIGHBOUR_INITIAL_INCREMENT), m_convergence_threshold, m_learn_more_threshold, m_nodeip));
//        }
//      }
//    }
    m_neighbours.push_back(neighb);
  } else {
    NS_LOG_ERROR("I (" << m_nodeip << ") received a SendHello message from an AODV neighbour and it was already a neighbour. neighb= " << neighb << ". (3x bc traffic types.)");
  }
}

//	TODO HANS - Do we need to keep track of neighbours?
void QDeep::RemoveNeighbour(Ipv4Address neighb) {
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

void
QDeep::SetQValueWrapper(Ipv4Address dst, Ipv4Address next_hop, Time new_value) {
  ToFile(m_output_file_name);
  GetEntryByRef(dst,next_hop)->SetQValue(new_value);
  ToFile(m_output_file_name);
}

void
QDeep::ToFile(std::string filename) {
  if (m_out_stream == 0 || m_in_test || !m_print_qtables) {
    return;
  }

  if (this->fileEmpty(filename)) {
    for (auto i : m_destinations) {
      for (auto j : m_neighbours) {
        *(m_out_stream->GetStream ()) << i << " -> " << j << ",";
      }
    }
    *(m_out_stream->GetStream ()) << std::endl;
  }
  *(m_out_stream->GetStream ()) << Simulator::Now() << "|";
//  for (auto i : m_destinations) {
//    for (auto j : m_neighbours) {
//		TODO HANS - How to effectively print the data from the Neural network OR is there no alternative
//      *(m_out_stream->GetStream ())  << (*std::find_if( m_qtable[i].begin(), m_qtable[i].end(), [j](const QDecisionEntry* elt){ return elt->GetNextHop() == j;} ))->GetQValue() << ",";
//    }
//  }
  *(m_out_stream->GetStream ()) << std::endl;
}

void
QDeep::FinalFile() {
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
QDeep::CheckDestinationKnown(const Ipv4Address& dst) {
	// TODO HANS - potentially include a way to check if NN knows destination?
  if (!((dst == m_nodeip)|| std::find(m_destinations.begin(), m_destinations.end(), dst) != m_destinations.end())) {
    NS_LOG_DEBUG("CheckDestinationKnown " << dst << " " << m_nodeip << "  "
                  << (std::find(m_neighbours.begin(), m_neighbours.end(), dst) != m_neighbours.end()) << "    "
                  << (std::find(m_destinations.begin(), m_destinations.end(), dst) != m_destinations.end()));
  }
  return (dst == m_nodeip || std::find(m_destinations.begin(), m_destinations.end(), dst) != m_destinations.end());
}

bool
QDeep::AddDestination(Ipv4Address via, Ipv4Address dst, Time t) {
  NS_ASSERT_MSG(m_nodeip != dst, m_nodeip << " tried to add QRoute to itself. Let the debugging commence!");
  // HANS - originally this was a check if the destination was in the q table
  if (std::find(m_destinations.begin(), m_destinations.end(), dst) != m_destinations.end()) {
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
//    HANS - should I already 'initialize' the destination when using a NN?? Probably not
//    m_qtable[dst] = std::vector<QDecisionEntry* >();

//    TODO HANS - I don't think it is needed to know all your neighbours and initialize Q values for them
//    for (auto i : m_neighbours) {
//    	// you do not know via which neighbour to reach the new destination ...
//      if (via == Ipv4Address(IP_WHEN_NO_NEXT_HOP_NEIGHBOUR_KNOWN_YET)) {
//        m_qtable[dst].push_back(new QDeepEntry(i, MilliSeconds(INITIAL_QVALUE_QTABLEENTRY_VIA), m_convergence_threshold, m_learn_more_threshold, m_nodeip)); //this is dodgy too ...
//      } else {
//    	  // the new destination should be reached through the current node
//        if (i == via) {
//          m_qtable[dst].push_back(new QDeepEntry(i, t,m_convergence_threshold, m_learn_more_threshold, m_nodeip));
//        } else { // some initial estimates i guess
//          m_qtable[dst].push_back(new QDeepEntry(i, t + MilliSeconds(INITIAL_QVALUE_QTABLEENTRY_NOT_VIA), m_convergence_threshold, m_learn_more_threshold, m_nodeip)); //Actually no idea...
//        }
//      }
//    }
    return true;
  }
}

QDeep::~QDeep() { }

std::string
QDeep::PrettyPrint(std::string extra_info) {
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
  // HANS - I removed the & sign here

//  TODO HANS - Fix prettyprint function for Neural Network
//  for (const auto dst : m_destinations) {
//    for (unsigned int i = 0; i < m_qtable[dst].size(); i++) {
//      m_qtable[dst].at(i)->GetNextHop().Print(help);
//      if (std::find(m_unavail.begin(), m_unavail.end(), m_qtable[dst].at(i)->GetNextHop() ) != m_unavail.end()) {
//        help << " (U)";
//      }
//      tmp = help.str();
//      help.str(std::string());
//      tmp.insert(0, 15-tmp.size(), ' ');
//      oss << tmp << "    |   ";
//    }
//    oss << "      " << extra_info << std::endl;
//    break;
//  }

  // Print the rows : dst | estim | estim | estim
  // HANS - I removed the & sign here
  for (const auto dst : m_destinations) {
    help << dst;
    tmp = help.str();
    help.str(std::string());
    tmp.insert(0, 15-tmp.size(), ' ');

    oss << tmp << "   |   ";

//    for (unsigned int i = 0; i < m_qtable[dst].size(); i++) {
//      help << m_qtable[dst].at(i)->GetQValue().As (Time::MS);
//      tmp = help.str();
//      help.str(std::string());
//
//      if (m_qtable[dst].at(i)->HasConverged() ) {
//        tmp_C = "(C) ";
//      } else if (m_qtable[dst].at(i)->IsBlackListed() ) {
//        tmp_C = "(B) ";
//      } else {
//        tmp_C = "";
//      }
//
//      std::size_t found_decimal = tmp.find(".");
//      std::size_t found_ms = tmp.find("ms");
//
//      NS_ASSERT_MSG(tmp[0] == '+', "expecting there to be a + at the start of input, otherwise undefined behaviour.");
//      tmp = tmp.substr(1,found_decimal) + tmp.substr(found_decimal+1, 3);
//      if (found_ms > found_decimal+4) {
//        tmp += "ms";
//      }
//
//      if ((tmp_C.size() +tmp.size()) < 15) {
//        tmp.insert(0, 15-(tmp.size()+tmp_C.size()), ' ');
//      }
//      oss << tmp_C << tmp << " " << "   |   ";
//
//    }
    oss << std::endl;
  }
  oss << std::endl;
  return oss.str();
}

//	TODO HANS - Assert statement without m_table
//	TODO HANS - What does a get entry call need to return in the case of a Neural Network?
QDecisionEntry* QDeep::GetEntryByRef(Ipv4Address dst,Ipv4Address via) {
	return new QDeepEntry();
//  NS_ASSERT_MSG(std::find_if(m_qtable[dst].begin(), m_qtable[dst].end(), [via](QDecisionEntry* obj) {
//	  return obj->GetNextHop() == via;
//    } ) != m_qtable[dst].end(),
//  "\nTried to find an entry by reference but it did not exist!? dst=" << dst << " via="<<via<< " and i am " << m_nodeip << std::endl << PrettyPrint() );
//
//  return *(std::find_if(m_qtable[dst].begin(), m_qtable[dst].end(), [via](const QDecisionEntry* obj) {return obj->GetNextHop() == via; } ) );
}

//	TODO HANS - Fix assert statement
//	TODO HANS - Return an actual QDeepEntry
QDecisionEntry*
QDeep::GetNextEstimToLearn(Ipv4Address dst) {
//  NS_ASSERT_MSG(m_qtable.find(dst) != m_qtable.end() || dst == m_nodeip, "(best estim) If we encounter a dst we have previously not registered we will get strange behaviour. (right?)");
  QDecisionEntry *q = new QDeepEntry();
  q->SetNodeIp(m_nodeip);

  if (dst == m_nodeip) {
    return new QDeepEntry(q->GetNextHop(),Seconds(0), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

//	TODO HANS - Check what this does and how to replicate it with a NN
//  for (const auto elt : m_qtable[dst]) {
//	  // when elt has converged or when the next hop of elt IS available, do nothing
//    if (  (elt->HasConverged() ) ||
//          (std::find(m_unavail.begin(),m_unavail.end(),elt->GetNextHop()) != m_unavail.end())
//       ) {
//      //do nothing
//    } else {
//      q = elt;
//    }
//  }

  bool any_neighb_reachable = false;
  // HANS - I removed the & sign here
  for (const auto j : m_neighbours) {
	  // if the current neighbour j is unavailable, so far, there is no available neighbour yet
    if ( std::find(m_unavail.begin(), m_unavail.end(), j) != m_unavail.end() ) {
      any_neighb_reachable = false || any_neighb_reachable;
      // NS_ASSERT_MSG(false, "ALL NEIGHBOURS ARE UNREACHABLE");
    } else {
    	// one neighbour that is available is enough
      any_neighb_reachable = true;
    }
  }

  if (!any_neighb_reachable) {
    return new QDeepEntry(Ipv4Address(NO_NEIGHBOURS_REACHABLE_ROUTE_IP), MilliSeconds(NO_NEIGHBOURS_REACHABLE_ROUTE_MS), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

  NS_ASSERT_MSG(q->GetQValue() != Years(1), "Somehow, the estimate remained equal to Years(1) (init value) which is a problem and will mess up the sim. Location:" << m_nodeip <<  ".");
  return q;
}

//	TODO HANS - Fix assert message
QDecisionEntry*
QDeep::GetNextEstim(Ipv4Address dst, Ipv4Address via) {
//  NS_ASSERT_MSG(m_qtable.find(dst) != m_qtable.end() || dst == m_nodeip, "(best estim) If we encounter a dst we have previously not registered we will get strange behaviour. (right?)");

  QDecisionEntry *q = new QDeepEntry();
  q->SetNodeIp(m_nodeip);

  if (dst == m_nodeip) {
    return new QDeepEntry(q->GetNextHop(),Seconds(0), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

//	TODO HANS - Check what this does and how to replicate it with a NN
//  for (const auto elt : m_qtable[dst]) {
//    if (elt->GetNextHop() != via ||
//        (elt->GetQValue() <= q->GetQValue() && std::find(m_unavail.begin(),m_unavail.end(),elt->GetNextHop()) != m_unavail.end())) {
//      //do nothing
//    } else {
//      q = elt;
//    }
//  }

  bool any_neighb_reachable = false;
  // HANS - I removed the & sign here
  for (const auto j : m_neighbours) {
    if ( std::find(m_unavail.begin(), m_unavail.end(), j) != m_unavail.end() ) {
      any_neighb_reachable = false || any_neighb_reachable;
      // NS_ASSERT_MSG(false, "ALL NEIGHBOURS ARE UNREACHABLE");
    } else {
      any_neighb_reachable = true;
    }
  }

  if (!any_neighb_reachable) {
    return new QDeepEntry(Ipv4Address(NO_NEIGHBOURS_REACHABLE_ROUTE_IP), MilliSeconds(NO_NEIGHBOURS_REACHABLE_ROUTE_MS), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

  NS_ASSERT_MSG(q->GetQValue() != Years(1), "Somehow, the estimate remained equal to Years(1) (init value) which is a problem and will mess up the sim. Location:" << m_nodeip <<  ".");
  return q;
}
//	TODO HANS - Fix Assert statement
//	TODO HANS - Return an actual QDeepEntry
QDecisionEntry*
QDeep::GetNextEstim(Ipv4Address dst, Ipv4Address next_hop_a, Ipv4Address next_hop_b) {
//  NS_ASSERT_MSG(m_qtable.find(dst) != m_qtable.end() || dst == m_nodeip, "(best estim) If we encounter a dst we have previously not registered we will get strange behaviour. (right?)");

  QDecisionEntry *q = new QDeepEntry();
  q->SetNodeIp(m_nodeip);

  if (dst == m_nodeip) {
    return new QDeepEntry(q->GetNextHop(),Seconds(0), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

  //QDecisionEntry *q_by_ref = q;
  if (q->GetNextHop() != Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE) ) {
    //q_by_ref = GetEntryByRef(dst, q->GetNextHop());
  }

//  TODO HANS - Figure out what this does and replicate it using a NN
//  for (const auto elt : m_qtable[dst]) {
//    if (elt->GetNextHop() == next_hop_a || // if its next hop a
//        (elt->GetQValue() <= q->GetQValue() && std::find(m_unavail.begin(),m_unavail.end(),elt->GetNextHop()) != m_unavail.end()) ||
//        elt->GetQValue() > q->GetQValue() || // or if its a worse estimate
//        elt->GetNextHop() == next_hop_b || // or if its next hop b
//        q_by_ref->IsBlackListed() // or if its blacklisted, BUT FOR THAT WE NEED IT BY REF
//        ) {
//      //do nothing
//    } else {
//      q = elt;
//    }
//  }

  if (q->GetNextHop() == Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE) && q->GetQValue() == Years(10) ) {
    NS_ASSERT(false);
//	TODO HANS - Figure out what this does and replicate it using a NN
//    for (const auto elt : m_qtable[dst]) {
//      if (elt->GetNextHop() == next_hop_a ||
//          (elt->GetQValue() > q->GetQValue() && std::find(m_unavail.begin(),m_unavail.end(),elt->GetNextHop()) != m_unavail.end()) ||
//          q->IsBlackListed()
//          ) {
//        //do nothing
//      } else {
//        q = elt;
//      }
//    }
  }
  NS_ASSERT(!(q->GetNextHop() == Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE) && q->GetQValue() == Years(10)));

  bool any_neighb_reachable = false;
  // HANS - I removed the & sign here
  for (const auto j : m_neighbours) {
    if ( std::find(m_unavail.begin(), m_unavail.end(), j) != m_unavail.end() ) {
      any_neighb_reachable = false || any_neighb_reachable;
      // NS_ASSERT_MSG(false, "ALL NEIGHBOURS ARE UNREACHABLE");
    } else {
      any_neighb_reachable = true;
    }
  }

  if (!any_neighb_reachable) {
    return new QDeepEntry(Ipv4Address(NO_NEIGHBOURS_REACHABLE_ROUTE_IP), MilliSeconds(NO_NEIGHBOURS_REACHABLE_ROUTE_MS), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

  NS_ASSERT_MSG(q->GetQValue() != Years(1), "Somehow, the estimate remained equal to Years(1) (init value) which is a problem and will mess up the sim. Location:" << m_nodeip <<  ".");
  return q;
}


bool
QDeep::AllNeighboursBlacklisted(Ipv4Address dst) {
  NS_ASSERT_MSG(dst != Ipv4Address(NO_NEIGHBOURS_REACHABLE_ROUTE_IP), "At least be a real destination IP address if you're going to check this." );
  NS_ASSERT_MSG(dst != Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE), "At least be a real destination IP address if you're going to check this." );

  bool all_neighb_blacklisted = true;
  // HANS - I removed the & sign here
  for (const auto j : m_neighbours) {
	  // if a neighbour of mine is the destination, than not all neighbours are blacklisted
    if (dst == m_nodeip) {
      return false;
    } else{
    	// if my neighbour is unavailable AND the entry for dst & j IS NOT blacklisted && the entry is available
      if ( std::find(m_unavail.begin(), m_unavail.end(), j) == m_unavail.end() && !GetEntryByRef(dst,j)->IsBlackListed() && GetEntryByRef(dst,j)->IsAvailable() ) {
        all_neighb_blacklisted = false;
      } else {
    	  // the value of 'all neighbours blacklisted' stays the same
        all_neighb_blacklisted = true && all_neighb_blacklisted;
      }
    }
  }
  // if (all_neighb_blacklisted){
  //   std::cout << PrettyPrint() << std::endl;
  // }
  return all_neighb_blacklisted;
}

//	TODO HANS - Fix Assert statement
//	TODO HANS - Make sure an actual estimate is returned
QDecisionEntry*
QDeep::GetNextEstim(Ipv4Address dst) {
  /* if we cant find the destination in our qtable or if the destination is still ourselves ( if we're the PTST tagged packet's destination, this case happens ) */
//  NS_ASSERT_MSG(m_qtable.find(dst) != m_qtable.end() || dst == m_nodeip, "(best estim) If we encounter a dst we have previously not registered we will get strange behaviour. (right?)");

  QDecisionEntry *q = new QDeepEntry();
  q->SetNodeIp(m_nodeip);

  if (dst == m_nodeip) {
    return new QDeepEntry(q->GetNextHop(),Seconds(0), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

  if (AllNeighboursBlacklisted(dst) && m_nodeip == Ipv4Address("10.1.1.1") /* only the source sending randomly otherwise loops */ ) {
    // this is still experimental though
    // NS_ASSERT_MSG(false, "this part is yet to do");
    return GetRandomEstim(dst, false); //dont look at unconv only, just any random one will do..
  }

// TODO HANS - Figure out what this does and replicate it using a NN
//  for (const auto elt : m_qtable[dst]) {
//    if (elt->GetQValue() > q->GetQValue() || (elt->GetQValue() <= q->GetQValue() && std::find(m_unavail.begin(),m_unavail.end(),elt->GetNextHop()) != m_unavail.end())) {
//      //do nothing
//    } else {
//      q = elt;
//    }
//  }

  bool any_neighb_reachable = false;
  // HANS - I removed the & sign here
  for (const auto j : m_neighbours) {
    if ( std::find(m_unavail.begin(), m_unavail.end(), j) != m_unavail.end() ) {
      any_neighb_reachable = false || any_neighb_reachable;
      // NS_ASSERT_MSG(false, "ALL NEIGHBOURS ARE UNREACHABLE");
    } else {
      any_neighb_reachable = true;
    }
  }

  if (!any_neighb_reachable) {
    return new QDeepEntry(Ipv4Address(NO_NEIGHBOURS_REACHABLE_ROUTE_IP), MilliSeconds(NO_NEIGHBOURS_REACHABLE_ROUTE_MS), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

  NS_ASSERT_MSG(q->GetQValue() != Years(1), "Somehow, the estimate remained equal to Years(1) (init value) which is a problem and will mess up the sim. Location:" << m_nodeip <<  ".");
  return q;
}

// HANS - a node is only 'unblacklisted' when the number of strikes is ZERO...
void QDeepEntry::DeductStrike() {
  m_number_of_strikes -= 1;
  if (m_number_of_strikes == 0 && IsBlackListed()) {
    SetBlacklisted(false);
  }
}

// HANS - ... but a node is blacklisted when the number of strikes is MAX_NR_STRIKES...
// 			  does that make sense?
void QDeepEntry::AddStrike() {
  if (IsBlackListed()) {
    return;
  }
  m_number_of_strikes += 1;
  if (m_number_of_strikes == MAX_NR_STRIKES_BLACKLISTED_NODE && !IsBlackListed()) {
    SetBlacklisted(true);
  }
  NS_ASSERT_MSG(m_number_of_strikes <= MAX_NR_STRIKES_BLACKLISTED_NODE, "Shoudlnt be able to have more than 5 strikes.");
}


bool QDeep::LearnLess(Ipv4Address dst) {
  /* find best estimate, if that one says to learn less & some other alternative route agrees, then learn less */
  bool ret = false;
  QDecisionEntry *best_estim = GetNextEstim(dst);
  if (GetEntryByRef(dst,best_estim->GetNextHop())->LearnLess()) {
    for (const auto neighb : m_neighbours) {
    	// if a single entry says to learn less, there will be less learning traffic
      if (GetEntryByRef(dst,neighb)->LearnLess()) {
        ret = true;
      }
    }
  }
  // NS_LOG_UNCOND("Returning " << (ret ? "true":"false") << " in LearnLess for dst " << dst << " and best estim " << best_estim.GetNextHop());
  // NS_LOG_UNCOND(PrettyPrint());
  return ret || AllNeighboursBlacklisted(dst); //if all are blacklisted, its pointless to have a lot of learning traffic so lower it
}

bool QDeep::LearnMore(Ipv4Address dst) {
  /* find best estimate, if that one says to learn more then learn more */
  bool ret = false;
  QDecisionEntry *best_estim = GetNextEstim(dst);
  if (!AllNeighboursBlacklisted(dst)) { // if all neighbs are false, its not gonna happen!
    ret = GetEntryByRef(dst,best_estim->GetNextHop())->LearnMore();
    // what does the node with the best q value think? return this value
  }
  // NS_LOG_UNCOND("Returning " << (ret ? "true":"false") << " in LearnMore for dst " << dst << " and best estim " << best_estim.GetNextHop());
  // NS_LOG_UNCOND(PrettyPrint());
  return ret;
}

//	TODO HANS - fix Assert statement
//	TODO HANS - return an actual estimation
QDecisionEntry*
QDeep::GetRandomEstim(Ipv4Address dst, bool _unconverged_entries_only) {
  //bool unconverged_entries_only = _unconverged_entries_only;

  if (HasConverged(dst)) {
    //unconverged_entries_only = false;
  }
  /* if we cant find the destination in our qtable or if the destination is still ourselves ( if we're the PTST tagged packet's destination, this case happens ) */
//  NS_ASSERT_MSG(m_qtable.find(dst) != m_qtable.end() || dst == m_nodeip, "(random estim) If we encounter a dst we have previously not registered we will get strange behaviour. (right?)");

  Ipv4Address next_hop;

  if (dst == m_nodeip) {
	  // HANS - unused assert message? So this is acceptable behaviour?
    NS_ASSERT_MSG(false, "Ive been asked about the best route to myself, that shouldn't happen actually.");
    return new QDeepEntry( next_hop, Seconds(0), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
  }

  std::set<int> tried_indices;
  int random_index = 0;
  while (true) {
    random_index = rand()%(m_neighbours.size() );
//		TODO HANS - Figure out what this does and replicate it using a NN
//    if  (IsNeighbourAvailable(m_qtable[ dst ].at(random_index)->GetNextHop() ) && //if the nieghbour isnt available -> route is no good
//        ( (unconverged_entries_only && !m_qtable[ dst ].at(random_index)->HasConverged()) || !unconverged_entries_only)  ){ // if were only looking for non-converged values, skip this value
//     break;
//    }
    // HANS - isn't it possible that this way of randomly trying all indices can potentially
    // 		  take a long time. If multiple indices are tried multiple times before the last one
    tried_indices.insert(random_index);
    if (tried_indices.size() == m_neighbours.size()) {
      return new QDeepEntry(Ipv4Address(NO_NEIGHBOURS_REACHABLE_ROUTE_IP), MilliSeconds(NO_NEIGHBOURS_REACHABLE_ROUTE_MS), m_convergence_threshold, m_learn_more_threshold, m_nodeip);
      NS_ASSERT_MSG(false, "ALL NEIGHBOURS ARE UNREACHABLE");
    }
  }
//  return m_qtable[ dst ].at(random_index);
	return new QDeepEntry();
}

// TODO HANS - Just ignore the update function, because the back propagation of the NN takes care of this
void
QDeep::Update(Ipv4Address via, Ipv4Address dst, Time _queue_time_, Time travel_time, Time next_hop_estimate) {

}

// HANS - This function is called from within QosQLearner (not QLearner
uint64_t
QDeep::CalculateNewQValue(Ipv4Address via, Ipv4Address dst, Time _queue_time_, Time travel_time, Time next_hop_estimate) {

  AddDestination(via, dst, travel_time);

  Time queue_time = _queue_time_ ;

  Ipv4Address showme = Ipv4Address("10.1.1.5");
  Time old_value = Seconds(0);
  Time fix_rest= Seconds(0);
  Time new_value = Seconds(99);

// TODO HANS - Figure out what this does and replicate it with a NN
//  for (auto  q : m_qtable[dst]) {
//    if (via == q->GetNextHop()) {
//      old_value = q->GetQValue();
//      if (old_value == NanoSeconds(0)) {
//        queue_time = NanoSeconds(0);
//      }
//
//      if (m_nodeip == showme){
//        NS_LOG_DEBUG( "{ ONLY CALCULATING THE VALUE NOT UPDATING" );
//        NS_LOG_DEBUG( "  [node = " << m_nodeip << "] Received more information about a route from " << m_nodeip << " to " << dst << " via " << via << "." );
//        NS_LOG_DEBUG( "  Old estimate was " << q->GetQValue().As (Time::MS) << "  ((( " << q->GetQValue() << ")))");
//        // Given that P actually spent q units of time in x’s queue and s units in transmission to y, x can update its estimate s.t.:
//
//        // Time new_estim = Time(m_learningrate * (MilliSeconds(1) /* q */+ MilliSeconds(1) /* s */ + t - q.second).GetInteger());
//        NS_LOG_DEBUG( "  Trying to learn something with \n\t - queue time = " << _queue_time_.As(Time::MS) << " / if old is 0 : " << queue_time.As(Time::MS)
//                  << " \n\t - travel time = " << travel_time.As(Time::MS) << " \n\t - estimate received from nextHop = " << next_hop_estimate.As(Time::MS) << " \n\t - old estimate = " << q->GetQValue().As(Time::MS));
//      }
//
//      Time perceived_reward = travel_time + queue_time + next_hop_estimate;
//
//      // Time temp_q_for_discount_factor = Time( (1 - m_learningrate) * q->GetQValue().GetInteger()) + Time( ( m_learningrate * perceived_reward.GetInteger() ) ) ;
//      // QDeepEntry next_state_best = GetNextEstim(dst);
//      // if (temp_q_for_discount_factor > next_state_best->GetQValue() && via != next_state_best.GetNextHop() ) {
//      //   temp_q_for_discount_factor = next_state_best->GetQValue();
//      // }
//      new_value =    Time( (1 - m_learningrate) * q->GetQValue().GetInteger()) +
//                     Time( ( m_learningrate * ( perceived_reward.GetInteger() + m_gamma * next_hop_estimate.GetInteger() ) ) );
//
//      fix_rest  = q->GetQValue();
//      if (m_nodeip == showme) {
//        NS_LOG_DEBUG( "  Learned new estim = " << q->GetQValue().As(Time::MS) );
//        NS_LOG_DEBUG( "  Learned new estim = " << fix_rest );
//        NS_LOG_DEBUG(PrettyPrint());
//      }
//    }
//  }

//	TODO HANS - Figure out what this does and replicate it using a NN
// for (auto  q : m_qtable[dst]) {
//   if (q->GetQValue() == MilliSeconds(INITIAL_QVALUE_QTABLEENTRY_NOT_VIA)) {
//     NS_ASSERT_MSG(q->GetQValue() == MilliSeconds(1), "actually rather not have this happen.");
//     q->SetQValue(fix_rest + MilliSeconds(INITIAL_QVALUE_QTABLEENTRY_NOT_VIA));
//   }
// }

  if (m_nodeip == showme) {
    NS_LOG_DEBUG("After updating all values we obtain");
    NS_LOG_DEBUG(PrettyPrint());
    NS_LOG_DEBUG( "}" );
  }
  NS_ASSERT(new_value != Seconds(99));
  return new_value.GetInteger();
}


// TODO HANS - Just ignore ChangeQValuesFromZero (initialization of NN should include this)
void
QDeep::ChangeQValuesFromZero(Ipv4Address dst, Ipv4Address aodv_next_hop) {

}

} //namespace ns3
