/*
 * qdecisionfactory.h
 *
 *  Created on: Nov 15, 2019
 *      Author: hans
 */

#ifndef SRC_APPLICATIONS_MODEL_QDECISIONFACTORY_H_
#define SRC_APPLICATIONS_MODEL_QDECISIONFACTORY_H_


#include "ns3/ipv4-address.h"
#include "ns3/qtable.h"
#include "ns3/qdecision.h"

namespace ns3 {

// TODO HANS - Big one --> QDeepLearning type toevoegen
class QFactory {
public:
	static QDecision* makeQDecision(){
		return (tableBased) ? new QTable() : new QTable();
	}
	static QDecision* makeQDecision(std::vector<Ipv4Address> _neighbours, Ipv4Address nodeip, float learning_rate, float convergence_threshold,
		    float learn_more_threshold, std::vector<Ipv4Address> _unavail, std::string addition, bool _in_test, bool print_qtables, float gamma){
		return (tableBased) ?
				new QTable(_neighbours, nodeip, learning_rate, convergence_threshold, learn_more_threshold, _unavail, addition, _in_test, print_qtables, gamma)
		: new QTable(_neighbours, nodeip, learning_rate, convergence_threshold, learn_more_threshold, _unavail, addition, _in_test, print_qtables, gamma);
	}
	static QDecisionEntry* makeQDecisionEntry(){
		return (tableBased) ? new QTableEntry() : new QTableEntry();
	}
	static QDecisionEntry* makeQDecisionEntry(Ipv4Address i, Time t, float f, float ff, Ipv4Address ip){
		return (tableBased) ? new QTableEntry(i, t, f, ff, ip) : new QTableEntry(i, t, f, ff, ip);
	}

private:
	static const bool tableBased = true;

};


}


#endif /* SRC_APPLICATIONS_MODEL_QDECISIONFACTORY_H_ */
