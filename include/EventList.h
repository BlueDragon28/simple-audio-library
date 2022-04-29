#ifndef SIMPLE_AUDIO_LIBRARY_EVENTLIST_H_
#define SIMPLE_AUDIO_LIBRARY_EVENTLIST_H_

#include "Common.h"
#include <string>
#include <variant>
#include <queue>
#include <mutex>

namespace SAL
{
/*
Class used to contain all the event send by
the user. Play, Pause, Stop, Open etc.
*/
class EventList
{
    EventList(const EventList&) = delete;
public:
    EventList();
    ~EventList();

    /*
    Push an event into the queue.
    */
    void push(EventType type, EventVariant data);

    /*
    Retrieve an event (if available) and
    remove it from the queue.
    */
    EventData get();

    /*
    Return true if queue is not empty.
    */
    inline bool containEvent() const;

private:
    std::queue<EventData> m_queue;
    std::mutex m_queueMutex;
};

inline bool EventList::containEvent() const {return !m_queue.empty();}
inline void EventList::push(EventType type, EventVariant data)
    {m_queue.push({type, data});}
}

#endif // SIMPLE_AUDIO_LIBRARY_EVENTLIST_H_