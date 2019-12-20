#ifndef TQLRNPACKET_H
#define TQLRNPACKET_H

#include <iostream>
#include "ns3/header.h"
#include "ns3/enum.h"
#include "ns3/ipv4-address.h"
#include <map>
#include "ns3/nstime.h"

namespace ns3 {
namespace tqlrn {

enum MessageType
{
  TQLRNTYPE_HELLO = 1,   //!< TQLRNTYPE_HELLO
  TQLRNTYPE_HELLO_ACK = 2,
};

/**
* \ingroup tqlrn
* \brief TQLRN types
*/
class TypeHeader : public Header
{
public:
  /// c-tor
  TypeHeader (MessageType t = TQLRNTYPE_HELLO);

  // Header serialization/deserialization
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;

  /// Return type
  MessageType Get () const { return m_type; }
  /// Check that type if valid
  bool IsValid () const { return m_valid; }
  bool operator== (TypeHeader const & o) const;
private:
  MessageType m_type;
  bool m_valid;
};

std::ostream & operator<< (std::ostream & os, TypeHeader const & h);

/**
* \ingroup tqlrn
* \brief   Route Request (RREQ) Message Format
  \verbatim
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                            HELLO ID                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                    Destination IP Address  (BCAST)            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                    Originator IP Address                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \endverbatim
*/
class HelloHeader : public Header
{
public:
  /// c-tor
  HelloHeader (uint32_t requestID = 0,
               Ipv4Address dst = Ipv4Address (), Ipv4Address origin = Ipv4Address ());

  // Header serialization/deserialization
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;

  // Fields
  void SetId (uint32_t id) { m_requestID = id; }
  uint32_t GetId () const { return m_requestID; }
  void SetDst (Ipv4Address a) { m_dst = a; }
  Ipv4Address GetDst () const { return m_dst; }
  void SetOrigin (Ipv4Address a) { m_origin = a; }
  Ipv4Address GetOrigin () const { return m_origin; }

  // Flags
  // void SetGratiousRrep (bool f);
  // bool GetGratiousRrep () const;
  // void SetDestinationOnly (bool f);
  // bool GetDestinationOnly () const;
  // void SetUnknownSeqno (bool f);
  // bool GetUnknownSeqno () const;

  bool operator== (HelloHeader const & o) const;

private:
  uint32_t       m_requestID;      ///< RREQ ID
  Ipv4Address       m_dst;       ///< Destination Sequence Number
  Ipv4Address       m_origin;    ///< Source Sequence Number
};

std::ostream & operator<< (std::ostream & os, HelloHeader const &);
} /* namespace tqlrn */
} /* namespace ns3 */
#endif /* TQLRNPACKET_H */
