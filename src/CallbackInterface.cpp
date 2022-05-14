#include "CallbackInterface.h"

namespace SAL
{
CallbackInterface::CallbackInterface()
{}

CallbackInterface::~CallbackInterface()
{}

/*
Add a start file callback to the list of callback.
This callback if called when a (new) file start to play.
*/
void CallbackInterface::addStartFileCallback(StartFileCallback callback)
{
    std::scoped_lock lock(m_startFileCallbackMutex);
    m_startFileCallback.push_back(callback);
}

/*
Add a end file callback to the list of callback.
This callback if called when a (new) file start to play.
*/
void CallbackInterface::addEndFileCallbacl(EndFileCallback callback)
{
    std::scoped_lock lock(m_endFileCallbackMutex);
    m_endFileCallback.push_back(callback);
}

/*
Calling start file callback.
This event is store inside a list and is then call
from the main loop of the AudioPlayer class.
*/
void CallbackInterface::callStartFileCallback(const std::string& filePath)
{
    std::scoped_lock lock(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::START_FILE, filePath});
}

/*
Calling end file callback.
This event is store inside a list and is then call
from the main loop of the AudioPlayer class.
*/
void CallbackInterface::callEndFileCallback(const std::string& filePath)
{
    std::scoped_lock lock(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::END_FILE, filePath});
}
}