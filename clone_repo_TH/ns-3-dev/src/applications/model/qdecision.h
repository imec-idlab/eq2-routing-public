/*
 * QDecision.h
 *
 *  Created on: Nov 8, 2019
 *      Author: hans
 */

#ifndef QDECISION_H_
#define QDECISION_H_

#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/log.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/thomas-configuration.h"
#include "ns3/simulator.h"
#include "ns3/make-functional-event.h"
#include <algorithm>
#include <fstream>
#include <vector>
#include <map>
#include <cstdlib>

namespace ns3 {

class QDecisionEntry {
public:
  virtual ~QDecisionEntry(){};
  // TODO HANS - move implementation to qtable, so qdeep has its own implementation without this attribute
  Ipv4Address GetNextHop() const { return m_next_hop; }

  // TODO HANS - move implementation to qtable, so qdeep has its own implementation without this attribute
  Time GetQValue() const { return m_my_estim; }
  virtual void SetQValue(Time t, bool verbose = false) = 0;
  uint64_t GetRealDelay() const { return m_real_observed_delay; }
  void SetRealDelay(uint64_t t) { m_real_observed_delay = t; }
  uint16_t GetRealLoss() const { return m_real_observed_loss; }
  void SetRealLoss(uint16_t t) { m_real_observed_loss = t; }
  virtual void SetSenderConverged(bool b) = 0;
  void SetUnavailable(bool b) { m_unavailable = b; }
  bool IsAvailable() { return !m_unavailable; }
  bool HasConverged() const { return m_converged && m_sender_converged && !m_unavailable; }
  virtual void Unconverge() = 0;
  void SetCoefficientTally(float f) { m_coeff_tally = (f < 1.0 ? 1.0:f); }
  float GetCoefficientTally() { return m_coeff_tally; }
  virtual bool LearnMore() = 0;
  virtual bool LearnLess() = 0;
  void SetNodeIp(Ipv4Address ip) { m_my_ip = ip;}
  bool IsBlackListed() { return m_blacklisted; }
  void SetBlacklisted(bool b) { m_blacklisted = b; }
  int GetNrOfStrikes() { return m_number_of_strikes; }
  virtual void AddStrike() = 0;
  virtual void DeductStrike() = 0;
protected:
  QDecisionEntry(){};
  QDecisionEntry(Ipv4Address i, Time t, float f, float ff, Ipv4Address ip) :
      m_next_hop(i),
      m_my_estim(t),
      m_conv_threshold(f),
      m_learn_more_threshold(ff),
      m_my_ip(ip){}

  Ipv4Address m_next_hop;
  Time m_my_estim;
  uint64_t m_real_observed_delay;
  uint16_t m_real_observed_loss;
  float m_coeff_tally;
  bool m_converged;
  bool m_sender_converged;
  bool m_unavailable;
  bool m_learn_more;
  bool m_learn_less;
  float m_conv_threshold;
  float m_learn_more_threshold;
  bool m_allow_change_to_learning;
  bool m_blacklisted;
  Ipv4Address m_my_ip;
  int m_number_of_strikes;
};

class QDecision {
public:
	virtual ~QDecision(){};
// true if destination is already in our list of destinations
	virtual bool CheckDestinationKnown(const Ipv4Address& i) = 0;

	virtual bool AddDestination(Ipv4Address via, Ipv4Address dst, Time t) = 0;

	virtual void Update(Ipv4Address, Ipv4Address, Time, Time, Time) = 0;
//used exclusively in QoS QLearner to converge more easily ...
	virtual uint64_t CalculateNewQValue(Ipv4Address, Ipv4Address, Time, Time, Time) =0;

	virtual QDecisionEntry* GetNextEstim(Ipv4Address) = 0; //{return QDecisionEntry();};
	virtual QDecisionEntry* GetNextEstim(Ipv4Address, Ipv4Address) = 0; //{return QDecisionEntry();};
	virtual QDecisionEntry* GetNextEstim(Ipv4Address, Ipv4Address, Ipv4Address) = 0; // {return QDecisionEntry();};
	virtual QDecisionEntry* GetNextEstimToLearn(Ipv4Address) = 0; // {return QDecisionEntry();};
	virtual QDecisionEntry* GetRandomEstim(Ipv4Address, bool = false) = 0; // {return QDecisionEntry();};
	virtual QDecisionEntry* GetEntryByRef(Ipv4Address, Ipv4Address) = 0; // {return *(new QDecisionEntry());};

	virtual bool AllNeighboursBlacklisted(Ipv4Address)  = 0;
	virtual std::string PrettyPrint(std::string = "") =0;
	void PrettyPrintToCout() {
		std::cout << PrettyPrint();
	}
	virtual void ToFile(std::string filename) = 0;
	virtual void FinalFile() =  0;
	virtual void ChangeQValuesFromZero(Ipv4Address dst, Ipv4Address aodv_next_hop) = 0;
	virtual void MarkNeighbDown(Ipv4Address) = 0;
	virtual void AddNeighbour(Ipv4Address) = 0;

	virtual bool IsNeighbourAvailable(Ipv4Address) = 0;

	virtual bool HasConverged(Ipv4Address dst, bool = false) = 0;

	virtual void Unconverge() = 0;

	virtual bool LearnLess(Ipv4Address) = 0;
	virtual bool LearnMore(Ipv4Address) = 0;

	virtual void SetQValueWrapper(Ipv4Address, Ipv4Address, Time) = 0;

//For test...
	std::vector<Ipv4Address> GetNeighbours() {
		return m_neighbours;
	}
	std::vector<Ipv4Address> GetUnavails() {
		return m_unavail;
	}
//	virtual std::vector<QDecisionEntry*> GetEstims(Ipv4Address dst) = 0;
protected:
	QDecision(){};
	QDecision(std::vector<Ipv4Address> _neighbours, Ipv4Address nodeip, float learning_rate, float convergence_threshold,
    float learn_more_threshold, std::vector<Ipv4Address> _unavail, std::string addition,bool _in_test, bool print_qtables, float gamma) :
		m_nodeip(nodeip),m_learningrate(learning_rate), m_convergence_threshold(convergence_threshold), m_gamma(gamma), m_learn_more_threshold(learn_more_threshold),
		m_neighbours(_neighbours), m_destinations( _neighbours), m_unavail(_unavail), m_in_test(_in_test), m_print_qtables(print_qtables)
	{};

	virtual void RemoveNeighbour(Ipv4Address)  = 0;
// std::map<Ipv4Address, std::vector<std::pair<Ipv4Address, Time> > > m_qtable;

	Ipv4Address m_nodeip;
	float m_learningrate;
	float m_convergence_threshold;
	float m_gamma;
	float m_learn_more_threshold;
	std::string m_output_file_name;
	std::vector<Ipv4Address> m_neighbours;
	std::vector<Ipv4Address> m_destinations;
	std::vector<Ipv4Address> m_unavail;
	Ptr<OutputStreamWrapper> m_out_stream;
	bool m_in_test;
	bool m_print_qtables;
	std::string lengthen_string(std::string str, int new_len) {

	  return "";
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
};


} //namespace ns3

#endif /* QDECISION_H_ */
