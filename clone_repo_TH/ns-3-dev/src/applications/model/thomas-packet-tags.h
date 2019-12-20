#ifndef PTS_TAG_H
#define PTS_TAG_H

#include "ns3/tag.h"
#include "ns3/nstime.h"
#include "ns3/thomas-configuration.h"
namespace ns3 {

class RandomDecisionTag : public Tag {
public:
  RandomDecisionTag (bool random_next_hop = false) : Tag (), m_random_next_hop(random_next_hop) {}

  static TypeId GetTypeId ()
  {
    static TypeId tid = TypeId ("ns3::RandomDecisionTag")
      .SetParent<Tag> ()
      .SetGroupName("Application")
      .AddConstructor<RandomDecisionTag> ()
    ;
    return tid;
  }

  TypeId  GetInstanceTypeId () const {
    return GetTypeId ();
  }

  void SetRandomNextHop (bool b) {
    m_random_next_hop = b;
  }

  bool GetRandomNextHop () {
    return m_random_next_hop;
  }

  uint32_t GetSerializedSize () const {
    return sizeof(uint8_t);
  }

  void  Serialize (TagBuffer i) const {
    i.WriteU8(m_random_next_hop);
  }

  void  Deserialize (TagBuffer i) {
    m_random_next_hop = i.ReadU8();
  }

  void  Print (std::ostream &os) const {
    os << PrettyPrint();
  }

  std::string PrettyPrint() const {
    std::stringstream oss;
    if (m_random_next_hop) {
      oss << "Prevhop decided randomly to send it to us.";
    } else {
      oss << "Prevhop's decision was not random.";
    }
    return oss.str();
  }

private:
  /// if it was randomly sent back or not
  bool m_random_next_hop;
};

class QLrnInfoTag : public Tag {
public:
  QLrnInfoTag (uint64_t o = -1, Ipv4Address prevHop = Ipv4Address(UNINITIALIZED_IP_ADDRESS_QINFO_TAG_VALUE), bool maint = false, bool usable_delay = true) :
    Tag (), m_recv_prev (o), m_prevhop(prevHop), m_maintenance(maint), m_usable_delay(usable_delay) {  }

  static TypeId GetTypeId ()
  {
    static TypeId tid = TypeId ("ns3::QLrnInfoTag")
      .SetParent<Tag> ()
      .SetGroupName("Application")
      .AddConstructor<QLrnInfoTag> ()
    ;
    return tid;
  }

  TypeId  GetInstanceTypeId () const {
    return GetTypeId ();
  }


  uint64_t GetTimeAsInt() const {
    return m_recv_prev;
  }
  Time GetTime() const {
    return Time::FromInteger(GetTimeAsInt(), Time::NS);
  }
  void SetTime(uint64_t _time) {
    m_recv_prev = _time;
  }

  bool GetMaint() { return m_maintenance; }
  void SetMaint(bool b) { m_maintenance = b;}

  bool GetUsableDelay() { return m_usable_delay; }
  void SetUsableDelay( bool b ) { m_usable_delay = b; }

  void SetPrevHop(Ipv4Address prevhop) {
    m_prevhop = prevhop;
  }
  Ipv4Address GetPrevHop() {
    return m_prevhop;
  }


  uint32_t GetSerializedSize () const {
    return sizeof(uint64_t) + sizeof(Ipv4Address) + sizeof(uint8_t) + sizeof(uint8_t);
  }

  void  Serialize (TagBuffer i) const {
    i.WriteU64 (m_recv_prev);

    uint8_t buf[4];
    m_prevhop.Serialize (buf);
    i.Write (buf, 4);

    i.WriteU8(m_maintenance);
    i.WriteU8(m_usable_delay);
  }

  void  Deserialize (TagBuffer i) {
    m_recv_prev = i.ReadU64 ();

    uint8_t buf[4];
    i.Read (buf, 4);
    m_prevhop = Ipv4Address::Deserialize (buf);

    m_maintenance = i.ReadU8 ();
    m_usable_delay = i.ReadU8 ();
  }

  void  Print (std::ostream &os) const {
    os << PrettyPrint();
  }

  std::string PrettyPrint() const {
    std::stringstream oss;
    oss << "QLrnInfoTag showing packet was a reply to pkt received from " << m_prevhop << ". Contains: " << m_recv_prev << " (time) and " << m_prevhop << " (prevhop).";
    return oss.str();
  }

private:
  /// time at which the packet we're observing was received at the previous hop
  /// (this is needed when sending back information to the previous hop, for it to know travel time)
  uint64_t m_recv_prev;
  /// previous hop ip address to send back the estimate information
  Ipv4Address m_prevhop;
  /// bool signifying if its learning traffic to learn a new route (and converge) or to maintain the route
  bool m_maintenance;
  /// bool signifying if this should be used for E2E delay metrics or not (if its due to a max_retry error, it will be much too high delay)
  bool m_usable_delay;
};

class PacketTimeSentTagPrecursorCT : public Tag {
public:
  PacketTimeSentTagPrecursorCT (uint64_t p = -1, uint64_t q =-1) :
    Tag (), m_src_initial_estim_typeC(p), m_sent_time_at_src(q) {}

  static TypeId GetTypeId () {
    static TypeId tid = TypeId ("ns3::PacketTimeSentTagPrecursorCT")
      .SetParent<Tag> ()
      .SetGroupName("Application")
      .AddConstructor<PacketTimeSentTagPrecursorCT> ()
    ;
    return tid;
  }
  TypeId  GetInstanceTypeId () const { return GetTypeId (); }

  uint64_t GetEstimTypeCAsInt() const { return m_src_initial_estim_typeC; }
  Time GetEstimTypeC() const { return Time::FromInteger(GetEstimTypeCAsInt(), Time::NS); }
  void SetEstimTypeC(uint64_t estim_c) { m_src_initial_estim_typeC = estim_c; }

  uint64_t GetSentTimeAsInt() const { return m_sent_time_at_src; }
  Time GetSentTime() const { return Time::FromInteger(GetSentTimeAsInt(), Time::NS); }
  void SetSentTime(uint64_t sent_time) { m_sent_time_at_src = sent_time; }

  uint32_t GetSerializedSize () const { return sizeof(uint64_t) + sizeof(uint64_t); }

  void  Serialize (TagBuffer i) const {
    i.WriteU64 (m_src_initial_estim_typeC);
    i.WriteU64 (m_sent_time_at_src);
  }

  void  Deserialize (TagBuffer i) {
    m_src_initial_estim_typeC = i.ReadU64 ();
    m_sent_time_at_src = i.ReadU64 ();
  }

  void  Print (std::ostream &os) const { os << PrettyPrint(); }

  std::string PrettyPrint() const {
    std::stringstream oss;
    oss << "PacketTimeSentTagPrecursor showing packet was sent originally at time = " << m_sent_time_at_src <<  " and type C estim was " <<  m_src_initial_estim_typeC << ".";
    return oss.str();
  }

private:
  uint64_t m_src_initial_estim_typeC;
  uint64_t m_sent_time_at_src;
};

class PacketTimeSentTagPrecursorAB : public Tag {
public:
  PacketTimeSentTagPrecursorAB (uint64_t p = -1, uint64_t q =-1) :
    Tag (), m_src_initial_estim_typeA(p), m_src_initial_estim_typeB(q) {}

  static TypeId GetTypeId () {
    static TypeId tid = TypeId ("ns3::PacketTimeSentTagPrecursorAB")
      .SetParent<Tag> ()
      .SetGroupName("Application")
      .AddConstructor<PacketTimeSentTagPrecursorAB> ()
    ;
    return tid;
  }
  TypeId  GetInstanceTypeId () const { return GetTypeId (); }

  uint64_t GetEstimTypeAAsInt() const { return m_src_initial_estim_typeA; }
  Time GetEstimTypeA() const { return Time::FromInteger(GetEstimTypeAAsInt(), Time::NS); }
  void SetEstimTypeA(uint64_t estim_a) { m_src_initial_estim_typeA = estim_a; }

  uint64_t GetEstimTypeBAsInt() const { return m_src_initial_estim_typeB; }
  Time GetEstimTypeB() const { return Time::FromInteger(GetEstimTypeBAsInt(), Time::NS); }
  void SetEstimTypeB(uint64_t estim_b) { m_src_initial_estim_typeB = estim_b; }

  uint32_t GetSerializedSize () const { return sizeof(uint64_t) + sizeof(uint64_t); }

  void  Serialize (TagBuffer i) const {
    i.WriteU64 (m_src_initial_estim_typeA);
    i.WriteU64 (m_src_initial_estim_typeB);
  }

  void  Deserialize (TagBuffer i) {
    m_src_initial_estim_typeA = i.ReadU64 ();
    m_src_initial_estim_typeB = i.ReadU64 ();
  }

  void  Print (std::ostream &os) const { os << PrettyPrint(); }

  std::string PrettyPrint() const {
    std::stringstream oss;
    oss << "PacketTimeSentTagPrecursorAB showing original typeA estim was " << m_src_initial_estim_typeA  << " and type B estim was " <<  m_src_initial_estim_typeB << ".";
    return oss.str();
  }

private:
  uint64_t m_src_initial_estim_typeA;
  uint64_t m_src_initial_estim_typeB;
};


/*
  solution to issue in QLrn packet loss tracking:
    node0 sends a pkt A with QLrnInfoTag to node2,
    node2 (incorrectly) routes 3 pkts back to 0
    node2 sends qLrnHeader back to node0
    node0 gets the three incorrectly routed packets and counts them, since he routes them to node2
    node0 gets the QLrnHeader, that now has too few packets and packet loss is incorrectly detected
*/
class PacketLossTrackingSentTimeQInfo : public Tag {
public:
  PacketLossTrackingSentTimeQInfo ( uint64_t p = -1 ) : m_sent_time(p) {}
  static TypeId GetTypeId ()
  {
    static TypeId tid = TypeId ("ns3::PacketLossTrackingSentTimeQInfo")
      .SetParent<Tag> ()
      .SetGroupName("Application")
      .AddConstructor<PacketLossTrackingSentTimeQInfo> ()
    ;
    return tid;
  }
  TypeId  GetInstanceTypeId () const {
    return GetTypeId ();
  }
  uint64_t GetSentTimeAsInt() const {
    return m_sent_time;
  }
  Time GetSentTime() const {
    return Time::FromInteger(GetSentTimeAsInt(), Time::NS);
  }
  void SetSentTime(uint64_t i ) {
    m_sent_time = i;
  }
  uint32_t GetSerializedSize () const {
    return sizeof(uint64_t);
  }

  void  Serialize (TagBuffer i) const {
    i.WriteU64 (m_sent_time);
  }

  void  Deserialize (TagBuffer i) {
    m_sent_time = i.ReadU64 ();
  }

  void  Print (std::ostream &os) const {
    os << PrettyPrint();
  }

  std::string PrettyPrint() const {
    std::stringstream oss;
    oss << "PACKETLOSSISSUETAG showing QLrnHeader (should be, anyway) packet was sent originally at time = " << m_sent_time << std::endl;
    return oss.str();
  }

private:
  /// time at which the packet we're observing was sent by the source
  /// (this is needed for the source to know when to start a new learning phase, if it's estimates are being completely wrong)
  uint64_t m_sent_time;
};

class PacketTimeSentTag : public Tag {
public:
  PacketTimeSentTag (uint64_t p = -1, uint64_t q =-1, Ipv4Address ip = Ipv4Address(UNINITIALIZED_IP_ADDRESS_VALUE_PTST)) :
    Tag (), m_sent_time(p), m_initial_estim(q) , m_prevhop (ip) { }

  static TypeId GetTypeId ()
  {
    static TypeId tid = TypeId ("ns3::PacketTimeSentTag")
      .SetParent<Tag> ()
      .SetGroupName("Application")
      .AddConstructor<PacketTimeSentTag> ()
    ;
    return tid;
  }

  TypeId  GetInstanceTypeId () const {
    return GetTypeId ();
  }

  uint64_t GetSentTimeAsInt() const {
    return m_sent_time;
  }
  Time GetSentTime() const {
    return Time::FromInteger(GetSentTimeAsInt(), Time::NS);
  }
  void SetSentTime(uint64_t sent_time) {
    m_sent_time = sent_time;
  }



  uint64_t GetInitialEstimAsInt() const {
    return m_initial_estim;
  }
  Time GetInitialEstim() const {
    return Time::FromInteger(GetInitialEstimAsInt(), Time::NS);
  }
  void SetInitialEstim(uint64_t init_estim) {
    m_initial_estim = init_estim;
  }

  void SetPrevHop(Ipv4Address prevhop) {
    m_prevhop = prevhop;
  }
  Ipv4Address GetPrevHop() {
    return m_prevhop;
  }

  uint32_t GetSerializedSize () const {
    return sizeof(uint64_t) + sizeof(Ipv4Address)+ sizeof(uint64_t);
  }

  void  Serialize (TagBuffer i) const {
    i.WriteU64 (m_sent_time);
    i.WriteU64 (m_initial_estim);
    uint8_t buf[4];
    m_prevhop.Serialize (buf);
    i.Write (buf, 4);
  }

  void  Deserialize (TagBuffer i) {
    m_sent_time = i.ReadU64 ();
    m_initial_estim = i.ReadU64 ();
    uint8_t buf[4];
    i.Read (buf, 4);
    m_prevhop = Ipv4Address::Deserialize (buf);
  }

  void  Print (std::ostream &os) const {
    os << PrettyPrint();
  }

  std::string PrettyPrint() const {
    std::stringstream oss;
    oss << "PacketTag showing packet was sent originally at time = " << m_sent_time << " with initial estim of " <<  m_initial_estim << ".";
    return oss.str();
  }

private:
  /// time at which the packet we're observing was sent by the source
  /// (this is needed for the source to know when to start a new learning phase, if it's estimates are being completely wrong)
  uint64_t m_sent_time;
  /// storing the initial estimate also in this value, alternative is to store it in src and wait for every packet to arrive, but this is easier : just store it with the packet or not ? Id say so...
  /// Quite possible that we end up reworking this to storing packetID - estimate in the source, getting back info every time a packet arrives at DST ...
  uint64_t m_initial_estim;
  /// to document who is forwarding this pkt s.t. dst can see for metrics purposes
  Ipv4Address m_prevhop;
};

class QRoutedTrafficPacketTag : public Tag {
public:
  QRoutedTrafficPacketTag (bool q_routed = false) :
    Tag (), m_q_routed(q_routed) {}

  static TypeId GetTypeId ()
  {
    static TypeId tid = TypeId ("ns3::QRoutedTrafficPacketTag")
      .SetParent<Tag> ()
      .SetGroupName("Application")
      .AddConstructor<QRoutedTrafficPacketTag> ()
    ;
    return tid;
  }

  TypeId  GetInstanceTypeId () const {
    return GetTypeId ();
  }

  bool PacketIsQRouted() const {
    return m_q_routed;
  }
  void SetQRouted(bool q_routed) {
    m_q_routed = q_routed;
  }

  uint32_t GetSerializedSize () const {
    return sizeof(uint8_t);
  }

  void  Serialize (TagBuffer i) const {
    i.WriteU8(m_q_routed);
  }

  void  Deserialize (TagBuffer i) {
    m_q_routed = i.ReadU8();
  }

  void  Print (std::ostream &os) const {
    os << PrettyPrint();
  }

  std::string PrettyPrint() const {
    std::stringstream oss;
    oss << "PacketTag signifying the packet is QRouted.";
    return oss.str();
  }
private:
  // if its being handled by Q or not
  bool m_q_routed;
};

class PortNrTag : public Tag {
public:
  PortNrTag (uint16_t port = 0, bool lrn = false, uint64_t pkt_number = 0) :
    Tag (), m_port(port), m_learning(lrn), m_packet_number(pkt_number) { }

  static TypeId GetTypeId ()
  {
    static TypeId tid = TypeId ("ns3::PortNrTag")
      .SetParent<Tag> ()
      .SetGroupName("Application")
      .AddConstructor<PortNrTag> ()
    ;
    return tid;
  }

  TypeId  GetInstanceTypeId () const {
    return GetTypeId ();
  }

  uint16_t GetDstPort() const { return m_port; }
  void SetDstPort(uint16_t port) { m_port = port; }

  bool GetLearningPkt() const { return m_learning; }
  void SetLearningPkt(bool lrn) { m_learning = lrn; }

  uint64_t GetPktNumber() const { return m_packet_number; }
  void SetPktNumber (uint64_t p) { m_packet_number = p; }

  uint32_t GetSerializedSize () const {
    return sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint64_t);
  }

  void  Serialize (TagBuffer i) const {
    i.WriteU16(m_port);
    i.WriteU8(m_learning);
    i.WriteU64(m_packet_number);
  }

  void  Deserialize (TagBuffer i) {
    m_port = i.ReadU16();
    m_learning = i.ReadU8();
    m_packet_number = i.ReadU64();
  }

  void  Print (std::ostream &os) const {
    os << PrettyPrint();
  }

  std::string PrettyPrint() const {
    std::stringstream oss;
    oss << "PacketTag signifying the packet is " << (!m_learning ?  "not" : "") << " learning traffic and is destined for " << int(m_port) << ".";
    return oss.str();
  }
private:
  // if its being handled by Q or not
  uint16_t m_port;
  bool m_learning;
  uint64_t m_packet_number;
};

class DropPacketTag : public Tag {
public:
  DropPacketTag (bool drop= false) : Tag (), m_drop(drop) { }

  static TypeId GetTypeId ()
  {
    static TypeId tid = TypeId ("ns3::DropPacketTag")
      .SetParent<Tag> ()
      .SetGroupName("Application")
      .AddConstructor<DropPacketTag> ()
    ;
    return tid;
  }

  TypeId  GetInstanceTypeId () const {
    return GetTypeId ();
  }

  bool GetDrop() const { return m_drop; }
  void SetDrop(bool drop) { m_drop = drop; }

  uint32_t GetSerializedSize () const {
    return sizeof(uint8_t);
  }

  void  Serialize (TagBuffer i) const {
    i.WriteU8(m_drop);
  }

  void  Deserialize (TagBuffer i) {
    m_drop = i.ReadU8();
  }

  void Print (std::ostream &os) const {
    os << PrettyPrint();
  }

  std::string PrettyPrint() const {
    std::stringstream oss;
    oss << "If this tag is attached, it has been attached in the QLearner::HandleRouteInput and should be dropped by AODV afterward.";
    return oss.str();
  }
private:
  bool m_drop;
};

} //namespace ns3

#endif /* PTS_TAG_H */
