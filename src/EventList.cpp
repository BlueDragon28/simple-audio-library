#include "EventList.h"

namespace SAL 
{
EventList::EventList()
{}

EventList::~EventList()
{}

/*
Retrieve an event (if available) and
remove it from the queue.
*/
EventData EventList::get()
{
    if (containEvent())
    {
        EventData data = m_queue.front();
        m_queue.pop();
        return data;
    }
    return {EventType::INVALID, EventVariant()};
}
}