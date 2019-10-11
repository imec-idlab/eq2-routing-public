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
#include "request-response-helper.h"
#include "ns3/request-response-server.h"
#include "ns3/request-response-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

RequestResponseServerHelper::RequestResponseServerHelper (uint16_t port)
{
  m_factory.SetTypeId (RequestResponseServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void
RequestResponseServerHelper::SetAttribute (
  std::string name,
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
RequestResponseServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
RequestResponseServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
RequestResponseServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
RequestResponseServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<RequestResponseServer> ();
  node->AddApplication (app);

  return app;
}

RequestResponseClientHelper::RequestResponseClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (RequestResponseClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

RequestResponseClientHelper::RequestResponseClientHelper (Address address)
{
  m_factory.SetTypeId (RequestResponseClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
}

void
RequestResponseClientHelper::SetAttribute (
  std::string name,
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
RequestResponseClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<RequestResponseClient>()->SetFill (fill);
}

void
RequestResponseClientHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
  app->GetObject<RequestResponseClient>()->SetFill (fill, dataLength);
}

void
RequestResponseClientHelper::SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength)
{
  app->GetObject<RequestResponseClient>()->SetFill (fill, fillLength, dataLength);
}

ApplicationContainer
RequestResponseClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
RequestResponseClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
RequestResponseClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
RequestResponseClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<RequestResponseClient> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
