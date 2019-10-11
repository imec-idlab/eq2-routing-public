#include "packettable.h"

namespace ns3 {

PacketTableEntry::PacketTableEntry() {
  m_enqueued_at = Simulator::Now();
  m_last_queue_time = NanoSeconds(0);
  m_packet_state = NONE;
  m_number_of_times_enqueued = 0;
}

void PacketTableEntry::Enqueue() {
  NS_ASSERT(m_packet_state == DEQUEUED || m_packet_state == NONE);
  m_number_of_times_enqueued+=1;
  m_enqueued_at = Simulator::Now();
  m_packet_state = ENQUEUED;
}

void PacketTableEntry::Dequeue() {
  NS_ASSERT(m_packet_state == ENQUEUED);
  m_last_queue_time = Simulator::Now() - m_enqueued_at;
  m_packet_state = DEQUEUED;
  m_enqueued_at = NanoSeconds(0);
}

int PacketTableEntry::GetNumberOfTimesSeen() {
  return m_number_of_times_enqueued;
}

// ====================================================================================================

PacketTable::PacketTable() : m_packet_info() {

}

PacketTable::~PacketTable(){

}

void
PacketTable::EnqueuePacket(uint64_t packetUid) {
  if ( m_packet_info.find(packetUid) == m_packet_info.end() ) {
    m_packet_info.insert(std::make_pair(packetUid, PacketTableEntry() ) );
  }
  m_packet_info[packetUid].Enqueue();
}

void
PacketTable::DequeuePacket(uint64_t packetUid) {
  //TODO : this if / else was needed b/c otherwise we would dequeue a packet after getting it as a Duplicate
  //this would lead to very large queue times, which is not desired, of course
  if ( m_packet_info.find(packetUid) == m_packet_info.end() ) {
    NS_FATAL_ERROR("Trying to dequeue a packet that was never queued to begin with.");
  }
  m_packet_info[packetUid].Dequeue( ) ;
}

Time PacketTable::GetPacketQueueTime(uint64_t packetUid) {
  // return NanoSeconds(0); //disables the queue delay thing
  return m_packet_info[packetUid].GetLastQueueTime();
}

int PacketTable::GetNumberOfTimesSeen(uint64_t packetUid) {
  return m_packet_info[packetUid].GetNumberOfTimesSeen();
}

} //namespace ns3
