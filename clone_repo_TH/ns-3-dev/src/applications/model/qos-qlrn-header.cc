#include "qos-qlrn-header.h"
#include "ns3/qlrn-header.h"

namespace ns3 {

QoSQLrnHeader::QoSQLrnHeader (uint64_t packetUid ,
                              uint64_t time_as_int,
                              uint64_t next_estim,
                              uint64_t real_observed_delay,
                              bool m_sender_converged,
                              Ipv4Address packet_dst,
                              uint16_t real_observed_loss,
                              uint32_t num_pkts_recv,
                              TrafficType t,
                              bool valid ): m_packet_id(packetUid),
                                            m_time_as_int(time_as_int),
                                            m_next_estim(next_estim),
                                            m_real_observed_delay(real_observed_delay),
                                            m_sender_converged(m_sender_converged),
                                            m_packet_dst(packet_dst),
                                            m_real_observed_loss(real_observed_loss),
                                            m_num_pkt_recv_from_prevhop(num_pkts_recv),
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
QoSQLrnHeader::GetTypeId () {
  static TypeId tid = TypeId ("ns3::QoSLrnHeader")
    .SetParent<Header> ()
    .SetGroupName("Applications")
    .AddConstructor<QoSQLrnHeader> ()
  ;
  return tid;
}

TypeId
QoSQLrnHeader::GetInstanceTypeId () const {
  return GetTypeId();
}
uint32_t
QoSQLrnHeader::GetSerializedSize () const {
  return sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t);
}
void
QoSQLrnHeader::Serialize (Buffer::Iterator start) const {
  start.WriteU64(m_time_as_int);
  start.WriteU64(m_packet_id);
  start.WriteU64(m_next_estim);
  start.WriteU64(m_real_observed_delay);

  uint8_t buf[4];
  m_packet_dst.Serialize (buf);
  start.Write (buf, 4);
  start.WriteU8(uint8_t(m_traffic_type));
  start.WriteU8(uint8_t(m_sender_converged));

  start.WriteU16(m_real_observed_loss);
  start.WriteU32(m_num_pkt_recv_from_prevhop);

}
uint32_t
QoSQLrnHeader::Deserialize (Buffer::Iterator start) {
  if (start.GetRemainingSize() < GetSerializedSize()) {
    m_valid = false;
    return 0;
  }
  Buffer::Iterator i = start;
  m_time_as_int = i.ReadU64 ();
  m_packet_id = i.ReadU64 ();
  m_next_estim = i.ReadU64();
  m_real_observed_delay = i.ReadU64();

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
  m_real_observed_loss = i.ReadU16();
  m_num_pkt_recv_from_prevhop = i.ReadU32();

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
QoSQLrnHeader::Print (std::ostream &os) const {
  os << "========QOSQLRNHEADER=========\n";
  os << "PacketID   = " << m_packet_id << std::endl;
  os << "time recv  = " << m_time_as_int << std::endl;
  os << "my estim   = " << m_next_estim << std::endl;
  os << "loss       = " << m_real_observed_loss << std::endl;
  os << "obs delay  = " << m_real_observed_delay << std::endl;
  os << (m_sender_converged? "sndr is conv":"sndr not conv") << std::endl;
  os << (m_valid?            "pkt is  valid":"pkt not valid") << std::endl;
  os << "pkt dst:   = " << m_packet_dst << std::endl;
  os << "For traffic type " << m_traffic_type << std::endl;}

} //namespace ns3
