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
    if (containEvents())
    {
        std::scoped_lock lock(m_queueMutex);
        EventData data = m_queue.front();
        m_queue.pop();
        return data;
    }
    return {EventType::INVALID, EventVariant()};
}
}