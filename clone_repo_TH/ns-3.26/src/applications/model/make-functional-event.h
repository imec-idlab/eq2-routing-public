/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef MAKE_FUNCTIONAL_EVENT_H
#define MAKE_FUNCTIONAL_EVENT_H

#include <ns3/event-impl.h>

namespace ns3 {

template <typename T>
EventImpl * MakeFunctionalEvent (T function)
{
  class EventMemberImplFunctional : public EventImpl
  {
public:
    EventMemberImplFunctional (T function)
      : m_function (function)
    {
    }
    virtual ~EventMemberImplFunctional ()
    {
    }
private:
    virtual void Notify (void)
    {
      m_function();
    }
    T m_function;
  } *ev = new EventMemberImplFunctional (function);
  return ev;
}

} // namespace ns3

#endif /* MAKE_FUNCTIONAL_EVENT_H */
