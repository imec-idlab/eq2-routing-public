#ifndef PACKETTABLE_H
#define PACKETTABLE_H

#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include <map>

namespace ns3 {

enum PacketState {
  NONE = 200,
  ENQUEUED  = 300,
  DEQUEUED  = 500,
};

class PacketTableEntry {
public:
  PacketTableEntry();
  int GetNumberOfTimesSeen();
  Time GetEnqueueTime() { return m_enqueued_at; }
  Time GetDequeueTime() { return m_dequeued_at; }
  // packets can be sent back, and would then possibly end up in the queue again, with a different queue time
  // we should be able to deal with this
  Time GetLastQueueTime() { return m_last_queue_time; }
  // PacketState GetPacketState() { return m_packet_state; }

  void SetPacketState(PacketState p) { m_packet_state = p; }

  void Enqueue() ;
  void Dequeue() ;
private:
  Time m_enqueued_at;
  Time m_dequeued_at;
  Time m_last_queue_time;
  PacketState m_packet_state;
  int m_number_of_times_enqueued;
};

class PacketTable {
public:
  PacketTable();
  ~PacketTable();
  void EnqueuePacket(uint64_t);
  void DequeuePacket(uint64_t);

  Time GetPacketQueueTime(uint64_t);
  int GetNumberOfTimesSeen(uint64_t);

private:
  std::map<uint64_t,PacketTableEntry > m_packet_info;
};

} //namespace ns3

#endif /* PACKETTABLE_H */
