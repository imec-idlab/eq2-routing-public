#ifndef TRAFFIC_TYPES_H_
#define TRAFFIC_TYPES_H_

#include "ns3/tag.h"
#include "ns3/nstime.h"
#include <map>

namespace ns3 {

enum TrafficType
{
  OTHER = 0,
  ICMP  = 1,
  WEB  = 2,
  VOIP  = 3,
  VIDEO = 4,
  UDP_ECHO = 5,
  TRAFFIC_A = 6,
  TRAFFIC_B = 7,
  TRAFFIC_C = 8,
};

class TrafficRequirements {
public:
  TrafficRequirements(float,float,float);
  TrafficRequirements() { NS_FATAL_ERROR("TrafficRequirements should not be default c'tor-ed."); }
  float GetJitterMax() { return m_jitter_max;}
  float GetDelayMax() { return m_delay_max;}
  float GetRandomLossMax() { return m_random_loss_percentage_max;}
private:
  float m_jitter_max;
  float m_delay_max;
  float m_random_loss_percentage_max;
};
/*

One-way latency         ms    20 to 100 (regional) / 90 to 300 (intercontinental)    |   20 to 100 (regional) / 90 to 400 (intercontinental)   |     20 to 500
Jitter (peak-to-peak)   ms    0 to 50                                                |    0 to 150                                             |      0 to 500
Sequential packet loss  ms    Random loss only                                       |   40 to 200                                             |  40 to 10'000
Rate of sequential loss s^–1  Random loss only                                       |      < 10–3                                             |       < 10 –1
Random packet loss      %     0 to 0.05                                              |      0 to 2                                             |       0 to 20
Reordered packets       %     0 to 0.001                                             |   0 to 0.01                                             |      0 to 0.1

*/

static std::map<TrafficType,TrafficRequirements> TrafficTypeReqsMap
  {
    std::make_pair (TRAFFIC_A, TrafficRequirements(50 * 1000000,100 * 1000000,0.0005)),
    std::make_pair (TRAFFIC_B, TrafficRequirements(150 * 1000000,100 * 1000000,0.02)),
    std::make_pair (TRAFFIC_C, TrafficRequirements(500 * 1000000,500 * 1000000,0.20)),
    std::make_pair (WEB, TrafficRequirements(500 * 1000000,500 * 1000000,20)),
    std::make_pair (VIDEO, TrafficRequirements(500 * 1000000,500 * 1000000,2)),
    std::make_pair (VOIP, TrafficRequirements(500 * 1000000,500 * 1000000,0.05)),
    std::make_pair (ICMP, TrafficRequirements(500 * 1000000,500 * 1000000,100)),
    std::make_pair (OTHER, TrafficRequirements(500 * 1000000,500 * 1000000,100)), //had to add this one due to "fix" of PortNrTag, AODV was now also being put in here
  };
}

#endif /* TRAFFIC_TYPES_H_ */
