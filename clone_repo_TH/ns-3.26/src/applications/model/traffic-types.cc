#include "ns3/traffic-types.h"

namespace ns3 {

TrafficRequirements::TrafficRequirements(float jitter,float delay, float loss_percentage) :
    m_jitter_max(jitter), m_delay_max(delay), m_random_loss_percentage_max(loss_percentage) { }

} //namespace ns3
