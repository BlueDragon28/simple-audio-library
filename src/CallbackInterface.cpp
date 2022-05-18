#include "CallbackInterface.h"
#include <iostream>

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
void CallbackInterface::addEndFileCallback(EndFileCallback callback)
{
    std::scoped_lock lock(m_endFileCallbackMutex);
    m_endFileCallback.push_back(callback);
}

/*
Add a stream position (in frames) change callback.
This callback is called when the position of the 
stream is changing.
*/
void CallbackInterface::addStreamPosChangeInFramesCallback(StreamPosChangeInFramesCallback callback)
{
    std::scoped_lock lock(m_streamPosChangeInFramesMutex);
    m_streamPosChangeInFramesCallback.push_back(callback);
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
Calling stream position (in frames) change callback.
*/
void CallbackInterface::callStreamPosChangeInFramesCallback(size_t streamPos)
{
    std::scoped_lock lock(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::STREAM_POS_CHANGE_IN_FRAME, streamPos});
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
        If it's a Stream pos change in frames, retrieving the position
        inside it and call the callback.
        */
        case CallbackType::STREAM_POS_CHANGE_IN_FRAME:
        {
            size_t streamPos;
            try
            {
                streamPos = std::get<size_t>(data.data);
            }
            catch (const std::bad_variant_access&)
            {
                continue;
            }
            streamPosChangeInFramesCallback(streamPos);
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

/*
Template function to call a callback with an argument.
This function call every callback of a given array and
remove the invalid callback.
*/
template<typename CallbackArrayType, typename ValueType>
inline void callbackCallTemplate(CallbackArrayType& callbackArray, const ValueType& value)
{
    std::vector<typename CallbackArrayType::iterator> indexToRemove;

    // Iterate through the callback list.
    for (typename CallbackArrayType::iterator it = callbackArray.begin();
         it != callbackArray.end();
         it++)
    {
        try
        {
            // Call the function/method stored inside the std::function.
            (*it)(value);
        }
        catch (const std::bad_function_call&)
        {
            // If the call failed, store the iterator inside the list to be removed.
            indexToRemove.push_back(it);
        }
    }

    // Removing the bad method.
    for (typename std::vector<typename CallbackArrayType::iterator>::const_reverse_iterator crit = indexToRemove.crbegin();
         crit != indexToRemove.crend();
         crit++)
    {
        callbackArray.erase(*crit);
    }
}

void CallbackInterface::startFileCallback(const std::string& filePath)
{
    callbackCallTemplate(m_startFileCallback, filePath);
}

void CallbackInterface::endFileCallback(const std::string& filePath)
{
    callbackCallTemplate(m_endFileCallback, filePath);
}

void CallbackInterface::streamPosChangeInFramesCallback(size_t streamPos)
{
    callbackCallTemplate(m_streamPosChangeInFramesCallback, streamPos);
}
}