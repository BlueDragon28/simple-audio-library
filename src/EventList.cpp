#include "EventList.h"
#include "DebugLog.h"

// Define CLASS_NAME to have the name of the class.
const std::string CLASS_NAME = "EventList";

namespace SAL 
{
EventList::EventList()
{}

EventList::~EventList()
{}

EventData EventList::get()
{
    SAL_DEBUG("Getting new event from the list")

    if (containEvents())
    {
        std::scoped_lock lock(m_queueMutex);
        EventData data = m_queue.front();
        m_queue.erase(m_queue.begin());
        return data;
    }
    return {EventType::INVALID, EventVariant()};
}

int EventList::waitEvent()
{
    SAL_DEBUG("Wait event")

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
    m_queue.push_back({EventType::WAIT_EVENT, ++max});
    return max;
}

bool EventList::isWaitEventIDPresent(int id) const
{
    SAL_DEBUG("Check if a wait event is present")

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
