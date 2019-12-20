#include "qlrn-header.h"
#include "ns3/qlrn-header.h"

namespace ns3 {


QLrnHeader::QLrnHeader (uint64_t packetUid ,
                        uint64_t time_as_int,
                        uint64_t next_estim,
                        bool m_sender_converged,
                        Ipv4Address packet_dst,
                        TrafficType t,
                        bool valid ): m_packet_id(packetUid),
                                      m_time_as_int(time_as_int),
                                      m_next_estim(next_estim),
                                      m_sender_converged(m_sender_converged),
                                      m_packet_dst(packet_dst),
                                      m_valid(valid)
  {
    if (t == WEB || t == ICMP || t == VIDEO || t == VOIP
        || t == UDP_ECHO || t == TRAFFIC_A || t == TRAFFIC_B
        || t == TRAFFIC_C || t == OTHER) {
      m_traffic_type = t;
    } else {
      NS_FATAL_ERROR("type of traffic not recognized...");
    }
  }

TypeId
QLrnHeader::GetTypeId () {
  static TypeId tid = TypeId ("ns3::QLrnHeader")
    .SetParent<Header> ()
    .SetGroupName("Applications")
    .AddConstructor<QLrnHeader> ()
  ;
  return tid;
}

TypeId
QLrnHeader::GetInstanceTypeId () const {
  return GetTypeId();
}
uint32_t
QLrnHeader::GetSerializedSize () const {
  return sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint8_t);
}
void
QLrnHeader::Serialize (Buffer::Iterator start) const {
  start.WriteU64(m_time_as_int);
  start.WriteU64(m_packet_id);
  start.WriteU64(m_next_estim);

  uint8_t buf[4];
  m_packet_dst.Serialize (buf);
  start.Write (buf, 4);
  start.WriteU8(uint8_t(m_traffic_type));
  start.WriteU8(uint8_t(m_sender_converged));

}
uint32_t
QLrnHeader::Deserialize (Buffer::Iterator start) {
  if (start.GetRemainingSize() < GetSerializedSize()) {
    m_valid = false;
    return 0;
  }
  Buffer::Iterator i = start;
  m_time_as_int = i.ReadU64 ();
  m_packet_id = i.ReadU64 ();
  m_next_estim = i.ReadU64();

  uint8_t buf[4];
  i.Read (buf, 4);
  m_packet_dst = Ipv4Address::Deserialize (buf);

  uint8_t type = i.ReadU8();
  if (type == WEB  || type == ICMP || type == VIDEO ||
      type == VOIP || type == UDP_ECHO || type == TRAFFIC_A ||
      type == TRAFFIC_B || type == TRAFFIC_C || type == OTHER) {
    m_traffic_type = TrafficType(type);
  } else {
    m_valid = false;
  }

  m_sender_converged = i.ReadU8();

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
QLrnHeader::Print (std::ostream &os) const {
  os << "========QLRNHEADER=========\n";
  os << "PacketID   = " << m_packet_id << std::endl;
  os << "time recv  = " << m_time_as_int << std::endl;
  os << "my estim   = " << m_next_estim << std::endl;
  os << (m_sender_converged? "sndr    conv":"sndr  no conv") << std::endl;
  os << (m_valid?            "pkt is  valid":"pkt not valid") << std::endl;
  os << "pkt dst:   = " << m_packet_dst << std::endl;
  os << "For traffic type " << m_traffic_type << std::endl;
}

} //namespace ns3
