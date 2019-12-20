#include "tqlrn-packet.h"

namespace ns3
{
namespace tqlrn
{

NS_OBJECT_ENSURE_REGISTERED (TypeHeader);

TypeHeader::TypeHeader (MessageType t) :
  m_type (t), m_valid (true)
{
}

TypeId
TypeHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::tqlrn::TypeHeader")
    .SetParent<Header> ()
    .SetGroupName("Tqlrn")
    .AddConstructor<TypeHeader> ()
  ;
  return tid;
}

TypeId
TypeHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
TypeHeader::GetSerializedSize () const
{
  return 1;
}

void
TypeHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 ((uint8_t) m_type);
}

uint32_t
TypeHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t type = i.ReadU8 ();
  m_valid = true;
  switch (type)
    {
    case TQLRNTYPE_HELLO:
    case TQLRNTYPE_HELLO_ACK:
      {
        m_type = (MessageType) type;
        break;
      }
    default:
      m_valid = false;
    }
  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
TypeHeader::Print (std::ostream &os) const
{
  switch (m_type)
    {
    case TQLRNTYPE_HELLO:
      {
        os << "HELLO";
        break;
      }
    case TQLRNTYPE_HELLO_ACK:
      {
        os << "HELLOACK";
        break;
      }
    default:
      os << "UNKNOWN_TYPE";
    }
}

bool
TypeHeader::operator== (TypeHeader const & o) const
{
  return (m_type == o.m_type && m_valid == o.m_valid);
}

std::ostream &
operator<< (std::ostream & os, TypeHeader const & h)
{
  h.Print (os);
  return os;
}

/*
 * HELLO msg
 */
HelloHeader::HelloHeader(uint32_t requestID,
            Ipv4Address dst, Ipv4Address origin):
      m_requestID(requestID), m_dst(dst), m_origin(origin)

{
}

TypeId HelloHeader::GetTypeId () {
  static TypeId tid = TypeId ("ns3::tqlrn::HelloHeader")
    .SetParent<Header>()
    .SetGroupName("Tqlrn")
    .AddConstructor<HelloHeader> ();
  return tid;
}

TypeId HelloHeader::GetInstanceTypeId () const {
  return GetTypeId();
}

uint32_t HelloHeader::GetSerializedSize () const {
  return 0;//TODO
}

void HelloHeader::Serialize (Buffer::Iterator start) const {
  start.WriteHtonU32 (m_requestID);
  // WriteTo (start, m_dst);
  // start.WriteHtonU32 (m_dstSeqNo);
  // WriteTo (start, m_origin);
}

uint32_t HelloHeader::Deserialize (Buffer::Iterator start) {
  Buffer::Iterator i = start;
  m_requestID = i.ReadNtohU32 ();
  // ReadFrom (i, m_dst);
  // ReadFrom (i, m_origin);

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void HelloHeader::Print (std::ostream &os) const {
  os << "RREQ ID " << m_requestID << " destination: ipv4 " << m_dst
     << " source: ipv4 " << m_origin;
}

bool HelloHeader::operator== (HelloHeader const & o) const {
  return (m_requestID == o.m_requestID &&
          m_dst == o.m_dst && m_origin == o.m_origin);
}

std::ostream &
operator<< (std::ostream & os, HelloHeader const & h) {
  h.Print (os);
  return os;
}

} /* namespace ns3 */
} /* namespace tqlrn */
