#include "CallbackInterface.h"
namespace SAL
{
CallbackInterface::CallbackInterface() :
    m_isReadyLastStatus(false)
{
    // Calling the isReadyChanged signal every time the state of
    // the simple-audio-library is changing.
    auto callIsReadyChanged = [this]()->void {
        callIsReadyChangedCallback(this->m_isReadyGetter());
    };
    addStartFileCallback(std::bind(callIsReadyChanged));
    addEndFileCallback(std::bind(callIsReadyChanged));
    addStreamPlayingCallback(callIsReadyChanged);
    addStreamStoppingCallback(callIsReadyChanged);
}

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
Add a stream position (in seconds) change callback.
This callback is called when the position of the stream
is changing.
*/
void CallbackInterface::addStreamPosChangeCallback(StreamPosChangeCallback callback)
{
    std::scoped_lock lock(m_streamPosChangeMutex);
    m_streamPosChangeCallback.push_back(callback);
}

/*
Add a stream paused callback.
This callback is called when the stream is paused.
*/
void CallbackInterface::addStreamPausedCallback(StreamPausedCallback callback)
{
    std::scoped_lock lock(m_streamPausedMutex);
    m_streamPausedCallback.push_back(callback);
}

/*
Add a stream playing callback.
This callback is called when the stream start playing or is resuming.
*/
void CallbackInterface::addStreamPlayingCallback(StreamPlayingCallback callback)
{
    std::scoped_lock lock(m_streamPlayingMutex);
    m_streamPlayingCallback.push_back(callback);
}

/*
Add a stream stopping callback.
This callback is called when a stream stop playing.
*/
void CallbackInterface::addStreamStoppingCallback(StreamStoppingCallback callback)
{
    std::scoped_lock lock(m_streamStoppingMutex);
    m_streamStoppingCallback.push_back(callback);
}

/*
Add a stream buffering callback.
This callback is called when the stream is buffering.
*/
void CallbackInterface::addStreamBufferingCallback(StreamBufferingCallback callback)
{
    std::scoped_lock lock(m_streamBufferingMutex);
    m_streamBufferingCallback.push_back(callback);
}

/*
Add a stream enough buffering callback.
This callback is called when the stream have finish buffering
*/
void CallbackInterface::addStreamEnoughBufferingCallback(StreamEnoughBufferingCallback callback)
{
    std::scoped_lock lock(m_streamEnoughBufferingMutex);
    m_streamEnoughBufferingCallback.push_back(callback);
}

/*
Add is ready changed callback.
This callback is called whenever the isReady is called.
*/
void CallbackInterface::addIsReadyChangedCallback(IsReadyChangedCallback callback)
{
    std::scoped_lock lock(m_isReadyChangedMutex);
    m_isReadyChangedCallback.push_back(callback);
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
Calling stream position (in seconds) change callback.
*/
void CallbackInterface::callStreamPosChangeCallback(size_t streamPos)
{
    std::scoped_lock lock(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::STREAM_POS_CHANGE, streamPos});
}

/*
Calling stream paused callback.
*/
void CallbackInterface::callStreamPausedCallback()
{
    std::scoped_lock lock(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::STREAM_PAUSED});
}

/*
Calling stream playing callback
*/
void CallbackInterface::callStreamPlayingCallback()
{
    std::scoped_lock lock(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::STREAM_PLAYING});
}

/*
Calling stream stopping callback.
*/
void CallbackInterface::callStreamStoppingCallback()
{
    std::scoped_lock lock(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::STREAM_STOPPING});
}

/*
Calling stream buffering callback.
*/
void CallbackInterface::callStreamBufferingCallback()
{
    std::scoped_lock lock(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::STREAM_BUFFERING});
}

/*
Calling stream enough buffering callback.
*/
void CallbackInterface::callStreamEnoughBufferingCallback()
{
    std::scoped_lock lock(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::STREAM_ENOUGH_BUFFERING});
}

/*
Calling the is ready changed callback.
*/
void CallbackInterface::callIsReadyChangedCallback(bool isReady)
{
    std::scoped_lock lock(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::IS_READY_CHANCHED, isReady});
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
        If it's a Stream pos change, retrieving the position inside it
        and call the callback.
        */
        case CallbackType::STREAM_POS_CHANGE:
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
            streamPosChangeCallback(streamPos);
        } break;

        /*
        If it's a stream paused, call the stream paused callback.
        */
        case CallbackType::STREAM_PAUSED:
        {
            streamPausedCallback();
        } break;

        /*
        If it's a stream playing, call the stream playing callback.
        */
        case CallbackType::STREAM_PLAYING:
        {
            streamPlayingCallback();
        } break;

        /*
        If it's a stream stopping, call the stream stopping callback.
        */
        case CallbackType::STREAM_STOPPING:
        {
            streamStoppingCallback();
        } break;

        /*
        If it's a stream buffering, call the stream buffering callback.
        */
        case CallbackType::STREAM_BUFFERING:
        {
            streamBufferingCallback();
        } break;

        /*
        If it's a stream enough buffering, call the stream enough buffering callback.
        */
        case CallbackType::STREAM_ENOUGH_BUFFERING:
        {
            streamEnoughBufferingCallback();
        } break;

        /*
        If it's a is ready changed, call the is ready changed callback.
        */
        case CallbackType::IS_READY_CHANCHED:
        {
            bool isReady;
            try
            {
                isReady = std::get<bool>(data.data);
            }
            catch (const std::bad_variant_access&)
            {
                continue;
            }
            if (isReady != m_isReadyLastStatus)
            {
                isReadyChangedCallback(isReady);
                m_isReadyLastStatus = isReady;
            }
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
template<typename CallbackArrayType, typename... Args>
inline void callbackCallTemplate(CallbackArrayType& callbackArray, const Args&... args)
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
            (*it)(args...);
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
    std::scoped_lock lock(m_startFileCallbackMutex);
    callbackCallTemplate(m_startFileCallback, filePath);
}

void CallbackInterface::endFileCallback(const std::string& filePath)
{
    std::scoped_lock lock(m_endFileCallbackMutex);
    callbackCallTemplate(m_endFileCallback, filePath);
}

void CallbackInterface::streamPosChangeInFramesCallback(size_t streamPos)
{
    std::scoped_lock lock(m_streamPosChangeInFramesMutex);
    callbackCallTemplate(m_streamPosChangeInFramesCallback, streamPos);
}

void CallbackInterface::streamPosChangeCallback(size_t streamPos)
{
    std::scoped_lock lock(m_streamPosChangeMutex);
    callbackCallTemplate(m_streamPosChangeCallback, streamPos);
}

void CallbackInterface::streamPausedCallback()
{
    std::scoped_lock lock(m_streamPausedMutex);
    callbackCallTemplate(m_streamPausedCallback);
}

void CallbackInterface::streamPlayingCallback()
{
    std::scoped_lock lock(m_streamPlayingMutex);
    callbackCallTemplate(m_streamPlayingCallback);
}

void CallbackInterface::streamStoppingCallback()
{
    std::scoped_lock lock(m_streamStoppingMutex);
    callbackCallTemplate(m_streamStoppingCallback);
}

void CallbackInterface::streamBufferingCallback()
{
    std::scoped_lock lock(m_streamBufferingMutex);
    callbackCallTemplate(m_streamBufferingCallback);
}

void CallbackInterface::streamEnoughBufferingCallback()
{
    std::scoped_lock lock(m_streamEnoughBufferingMutex);
    callbackCallTemplate(m_streamEnoughBufferingCallback);
}

void CallbackInterface::isReadyChangedCallback(bool isReady)
{
    std::scoped_lock lock(m_isReadyChangedMutex);
    callbackCallTemplate(m_isReadyChangedCallback, isReady);
}

/*
Set the is ready getter.
*/
void CallbackInterface::setIsReadyGetter(std::function<bool()> getter)
{
    m_isReadyGetter = getter;
}
}