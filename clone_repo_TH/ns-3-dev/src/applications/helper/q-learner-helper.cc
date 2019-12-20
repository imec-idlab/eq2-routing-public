/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,

 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "q-learner-helper.h"
#include "ns3/q-learner.h"
#include "ns3/qos-q-learner.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

QLearnerHelper::QLearnerHelper (float eps, float learning_rate, float gamma, float qconvergence, float rho, float m_lrn_more,  bool in_test,
                                int max_retry, bool ideal,bool use_lrn_ph, bool qos,
                                bool output_results, bool printQTables, bool reportMetrics) : m_qos(qos)
{
  if (qos) {
    m_factory.SetTypeId (QoSQLearner::GetTypeId ());
  } else {
    m_factory.SetTypeId (QLearner::GetTypeId ());
  }
  SetAttribute ("EpsilonThreshold", DoubleValue(eps));
  SetAttribute ("LearningRate", DoubleValue(learning_rate));
  SetAttribute ("Testing", BooleanValue(in_test));
  SetAttribute ("Ideal", BooleanValue(ideal));
  SetAttribute ("UseLearningPhases", BooleanValue(use_lrn_ph));
  SetAttribute ("Gamma", DoubleValue(gamma));
  SetAttribute ("OutputDataToFile", BooleanValue(output_results));
  SetAttribute ("PrintQTables", BooleanValue(printQTables));
  SetAttribute ("ReportPacketMetricsToSrc", BooleanValue(reportMetrics));
  SetAttribute ("Rho", DoubleValue(rho));
  SetAttribute ("QConvergenceThreshold", DoubleValue(qconvergence));
  SetAttribute ("MaxRetries", IntegerValue(max_retry));
  SetAttribute ("LearnMoreThresh", DoubleValue(m_lrn_more));
}

void
QLearnerHelper::SetAttribute (
  std::string name,
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
QLearnerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
QLearnerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
QLearnerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
QLearnerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app;
  if (m_qos) {
    app = m_factory.Create<QoSQLearner> ();
  } else {
    app = m_factory.Create<QLearner> ();
  }
  node->AddApplication (app);

  return app;
}

} // namespace ns3
