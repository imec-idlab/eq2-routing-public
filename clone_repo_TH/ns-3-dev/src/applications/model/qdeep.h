#ifndef QDEEP_H_
#define QDEEP_H_

#include "qdecision.h"
#include "tinn.h"
#include "genann.h"


namespace ns3 {

class QDeepEntry : public QDecisionEntry {
public:
  QDeepEntry();
  QDeepEntry(Ipv4Address, Time, float c_threshold, float lm_threshold, Ipv4Address myip);
  ~QDeepEntry();

  void SetQValue(Time t, bool verbose = false);
  void SetSenderConverged(bool b);
  void Unconverge();
  bool LearnMore();
  bool LearnLess();
  void AddStrike();
  void DeductStrike();
};

class QDeep : public QDecision {
public:
  QDeep();
  QDeep(std::vector<Ipv4Address>, Ipv4Address, float, float, float, std::vector<Ipv4Address>, std::string, bool, bool, float);
  ~QDeep();

  // true if destination is already in our list of destinations
  bool CheckDestinationKnown(const Ipv4Address& i);

  bool AddDestination(Ipv4Address via, Ipv4Address dst, Time t);

  void Update(Ipv4Address, Ipv4Address, Time, Time, Time);
  //used exclusively in QoS QLearner to converge more easily ...
  uint64_t CalculateNewQValue(Ipv4Address, Ipv4Address, Time, Time, Time);

  QDecisionEntry* GetNextEstim(Ipv4Address);
  QDecisionEntry* GetNextEstim(Ipv4Address,Ipv4Address);
  QDecisionEntry* GetNextEstim(Ipv4Address,Ipv4Address,Ipv4Address);
  QDecisionEntry* GetNextEstimToLearn(Ipv4Address);
  QDecisionEntry* GetRandomEstim(Ipv4Address,bool=false);
  QDecisionEntry* GetEntryByRef(Ipv4Address,Ipv4Address);

  bool AllNeighboursBlacklisted(Ipv4Address);
  std::string PrettyPrint(std::string="");
  void ToFile(std::string filename);
  void FinalFile();
  void ChangeQValuesFromZero(Ipv4Address dst, Ipv4Address aodv_next_hop) ;
  void MarkNeighbDown(Ipv4Address);
  void AddNeighbour(Ipv4Address);

  bool IsNeighbourAvailable(Ipv4Address);

  bool HasConverged(Ipv4Address dst, bool = false);

  void Unconverge();

  bool LearnLess(Ipv4Address);
  bool LearnMore(Ipv4Address);

  void SetQValueWrapper(Ipv4Address,Ipv4Address,Time);


//  std::vector<QDecisionEntry*> GetEstims(Ipv4Address dst) {
//	return new std::vector<QDecisionEntry*>();
//  }

private:
  void RemoveNeighbour(Ipv4Address);
//  TODO HANS - P1 - Comment out the q table and see what breaks!
//  std::map<Ipv4Address, std::vector<QDecisionEntry*> > m_qtable;
//  TinyNN qtnn;
  	genann gnn;
};

} //namespace ns3

#endif /* QDEEP_H */
