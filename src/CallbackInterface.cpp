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

/*
Call every callback inside the callback queue.
*/
void CallbackInterface::callback() 
{
    std::scoped_lock lock(m_callbackCallMutex);
    for (const CallbackData& data : m_callbackCall)
    {
        switch (data.type)
        {
        /*
        If it's a Start File callback, retrieving the
        string (filePath) inside it and call the start callback.
        */
        case CallbackType::START_FILE:
        {
            std::string filePath;
            try
            {
                filePath = std::get<std::string>(data.data);
            }
            catch (const std::bad_variant_access&)
            {
                continue;
            }
            startFileCallback(filePath);
        } break;

        /*
        If it's a End File callback, retrieving the string (filePath)
        inside it and call the end callback.
        */
        case CallbackType::END_FILE:
        {
            std::string filePath;
            try
            {
                filePath = std::get<std::string>(data.data);
            }
            catch (const std::bad_variant_access&)
            {
                continue;
            }
            endFileCallback(filePath);
        } break;

        /*
        If the callback type is unknown, do nothing.
        */
        case CallbackType::UNKNOWN:
        default:
        {
            continue;
        } break;
        }
    }
    m_callbackCall.clear();
}

void CallbackInterface::startFileCallback(const std::string& filePath)
{
    std::vector<std::vector<StartFileCallback>::iterator> indexToRemove;

    // Iterate through the start file callback list.
    for (std::vector<StartFileCallback>::iterator it = m_startFileCallback.begin();
         it != m_startFileCallback.end();
         it++)
    {
        try 
        {
            // Call the function/method stored inside the std::function.
            (*it)(filePath);
        }
        catch (const std::bad_function_call&)
        {
            // If the call failed, store the iterator inside a list to be removed.
            indexToRemove.push_back(it);
        }
    }

    // Removing the bad method.
    for (std::vector<std::vector<StartFileCallback>::iterator>::const_reverse_iterator crit = indexToRemove.crbegin();
         crit != indexToRemove.crend();
         crit++)
    {
        m_startFileCallback.erase(*crit);
    }
}

void CallbackInterface::endFileCallback(const std::string& filePath)
{
    std::vector<std::vector<EndFileCallback>::iterator> indexToRemove;

    // Iterate through the end file callback list.
    for (std::vector<EndFileCallback>::iterator it = m_endFileCallback.begin();
         it != m_endFileCallback.end();
         it++)
    {
        try
        {
            // Call the function/method stored inside the std::function.
            (*it)(filePath);
        }
        catch (const std::bad_function_call&)
        {
            // If the call failed, store the iterator inside the list to be removed.
            indexToRemove.push_back(it);
        }
    }

    // Removing the bad method.
    for (std::vector<std::vector<EndFileCallback>::iterator>::const_reverse_iterator crit = indexToRemove.crbegin();
         crit != indexToRemove.crend();
         crit++)
    {
        m_endFileCallback.erase(*crit);
    }
}
}