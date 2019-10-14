/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "tqlrn-helper.h"

namespace ns3 {

TQlrnHelper::TQlrnHelper() {
  m_agentFactory.SetTypeId("ns3::tqlrn::RoutingProtocol");
}

TQlrnHelper*
TQlrnHelper::Copy (void) const {
  return new TQlrnHelper(*this);
}


Ptr<Ipv4RoutingProtocol>
TQlrnHelper::Create (Ptr<Node> node) const{
  Ptr<tqlrn::RoutingProtocol> agent = m_agentFactory.Create<tqlrn::RoutingProtocol> ();
  node->AggregateObject (agent);
  return agent;
}


void
TQlrnHelper::Set (std::string name, const AttributeValue &value){
  m_agentFactory.Set (name, value);
}


int64_t
TQlrnHelper::AssignStreams (NodeContainer c, int64_t stream){
  NS_ASSERT_MSG (false, "TQlrnHelper::AssignStreams NOT IMPLEMENTED");
  // return (currentStream - stream);
  return 0;
}

}
