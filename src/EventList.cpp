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
        m_queue.erase(m_queue.begin());
        return data;
    }
    return {EventType::INVALID, EventVariant()};
}

/*
Return the max ID of WAIT_EVENT event.
*/
int EventList::maxWaitEvent() const
{
    std::scoped_lock lock(m_queueMutex);
    int max = 0;
    for (const EventData& data : m_queue)
    {
        if (data.type == EventType::WAIT_EVENT)
        {
            try
            {
                max = std::max(
                    std::get<int>(data.data),
                    max);
            }
            catch (const std::bad_variant_access&)
            {
                continue;
            }
        }
    }
    return max;
}

/*
Return true if WAIT_EVENT ID is in the queue.
*/
bool EventList::isWaitEventIDPresent(int id) const
{
    std::scoped_lock lock(m_queueMutex);
    for (const EventData& data : m_queue)
    {
        if (data.type == EventType::WAIT_EVENT)
        {
            try
            {
                if (std::get<int>(data.data) == id)
                    return true;
            }
            catch (const std::bad_variant_access&)
            {
                continue;
            }
        }
    }
    return false;
}
}