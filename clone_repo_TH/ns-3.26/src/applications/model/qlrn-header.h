#ifndef QLRN_HEADER_H
#define QLRN_HEADER_H

#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/thomas-packet-tags.h"
#include "ns3/thomas-configuration.h"
#include "ns3/traffic-types.h"
#include "ns3/nstime.h"
#include "ns3/type-id.h"

namespace ns3 {

//TODO add some information about the next hop chosen by the node ? ? ?
// in test 1, that would enable node 10.1.1.1 to also learn about 10.1.1.3 as the next hop
// even though its not adressing it & not communicating with it directly (via 10.1.1.2)

/**
* \brief   QLrn Header
  \verbatim
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |              Time packet arrived at the next hop              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                              ^                                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           Packet UID                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                              ^                                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           next estim                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                              ^                                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      DestinationAddress                       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |      Type     |    sendconv   |              Empty            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \endverbatim
*/
class QLrnHeader : public Header
{
public:
  /// c-tor
  QLrnHeader (uint64_t packetUid = 0, uint64_t time_as_int = 0, uint64_t next_estim = 0, bool m_sender_converged = false,
                Ipv4Address packet_dst = Ipv4Address(UNINITIALIZED_IP_ADDRESS_QLRN_HEADER), TrafficType t = OTHER, bool valid = true);

  // Header serialization/deserialization
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;

  // Fields
  void SetPktId (uint64_t id) { m_packet_id = id; }
  uint64_t GetPktId () const { return m_packet_id; }
  void SetTime (Time t) { m_time_as_int = t.GetInteger(); }
  uint64_t GetTime() { return m_time_as_int; }
  void SetPDst (Ipv4Address i) { m_packet_dst = i;}
  Ipv4Address GetPDst() { return m_packet_dst; }
  uint64_t GetNextEstim() { return m_next_estim; }
  void SetNextEstim(uint64_t next_estim ) { m_next_estim = next_estim; }
  TrafficType GetTrafficType() { return m_traffic_type; }
  void SetSenderConverged(bool b) { m_sender_converged = b; }
  bool GetSenderConverged() { return m_sender_converged; }

  bool IsValid() { return m_valid; }
  // bool operator== (QLrnHeader const & o) const;

private:
  uint64_t    m_packet_id;      /// uid of packet that triggered this header to be sent
  uint64_t    m_time_as_int;    /// time upon which the packet arrived. can be used to determine travel time
  uint64_t    m_next_estim;     /// next estimate
  // uint8_t     m_type; not really needed, just assign 8 bits after the 128 bits, then check if it is equal to 1
  bool        m_sender_converged; /// put this in qlrn info tag first, wrongly. must be here -- sender signifies to -> src neighbour if its converged Q
  Ipv4Address m_packet_dst;     /// TODO
  bool        m_valid;
  TrafficType m_traffic_type;
};

}

#endif /*QLRN_HEADER_H */
