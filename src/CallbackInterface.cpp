#include "CallbackInterface.h"
#include "DebugLog.h"

// Define CLASS_NAME to have the name of the class.
const std::string CLASS_NAME = "CallbackInterface";

namespace SAL
{
CallbackInterface::CallbackInterface() :
    m_isReadyLastStatus(false)
{
    // Calling the isReadyChanged signal every time the state of
    // the simple-audio-library is changing.
    auto callIsReadyChanged = [this]()->void {
        bool isReady = this->m_isReadyGetter();
        if (isReady != this->m_isReadyLastStatus)
        {
            std::scoped_lock lock(this->m_backgroundThreadMutex);
            this->m_backgroundThread.push_back(std::thread(
                &CallbackInterface::callIsReadyChangedCallback, this, isReady));
        }
    };
    addStartFileCallback(std::bind(callIsReadyChanged));
    addEndFileCallback(std::bind(callIsReadyChanged));
    addStreamPlayingCallback(callIsReadyChanged);
    addStreamStoppingCallback(callIsReadyChanged);
}

CallbackInterface::~CallbackInterface()
{
    // Join the threads.
    std::scoped_lock btLock(m_backgroundThreadMutex);
    for (int i = 0; i < m_backgroundThread.size(); i++)
    {
        if (m_backgroundThread.at(i).joinable())
            m_backgroundThread[i].join();
    }
}

void CallbackInterface::addStartFileCallback(StartFileCallback callback)
{
    SAL_DEBUG("Adding a start file callback")

    std::scoped_lock lock(m_startFileCallbackMutex);
    m_startFileCallback.push_back(callback);
}

void CallbackInterface::addEndFileCallback(EndFileCallback callback)
{
    SAL_DEBUG("Adding a end file callback")

    std::scoped_lock lock(m_endFileCallbackMutex);
    m_endFileCallback.push_back(callback);
}

void CallbackInterface::addStreamPosChangeCallback(StreamPosChangeCallback callback, TimeType timeType)
{
    SAL_DEBUG(std::string("Adding a stream pos in ") + (timeType == TimeType::FRAMES ? "frames" : "seconds") + std::string(" change callback"))

    if (timeType == TimeType::SECONDS)
    {
        std::scoped_lock lock(m_streamPosChangeMutex);
        m_streamPosChangeCallback.push_back(callback);
    }
    else
    {
        std::scoped_lock lock(m_streamPosChangeInFramesMutex);
        m_streamPosChangeInFramesCallback.push_back(callback);
    }
}

void CallbackInterface::addStreamPausedCallback(StreamPausedCallback callback)
{
    SAL_DEBUG("Add stream paused callback")

    std::scoped_lock lock(m_streamPausedMutex);
    m_streamPausedCallback.push_back(callback);
}

void CallbackInterface::addStreamPlayingCallback(StreamPlayingCallback callback)
{
    SAL_DEBUG("Add stream playing callback")

    std::scoped_lock lock(m_streamPlayingMutex);
    m_streamPlayingCallback.push_back(callback);
}

void CallbackInterface::addStreamStoppingCallback(StreamStoppingCallback callback)
{
    SAL_DEBUG("Add stream stopping callback")

    std::scoped_lock lock(m_streamStoppingMutex);
    m_streamStoppingCallback.push_back(callback);
}

void CallbackInterface::addStreamBufferingCallback(StreamBufferingCallback callback)
{
    SAL_DEBUG("Add stream buffering callback")

    std::scoped_lock lock(m_streamBufferingMutex);
    m_streamBufferingCallback.push_back(callback);
}

void CallbackInterface::addStreamEnoughBufferingCallback(StreamEnoughBufferingCallback callback)
{
    SAL_DEBUG("Add stream enough buffering callback")

    std::scoped_lock lock(m_streamEnoughBufferingMutex);
    m_streamEnoughBufferingCallback.push_back(callback);
}

void CallbackInterface::addIsReadyChangedCallback(IsReadyChangedCallback callback)
{
    SAL_DEBUG("Add is ready changed callback")

    std::scoped_lock lock(m_isReadyChangedMutex);
    m_isReadyChangedCallback.push_back(callback);
}

void CallbackInterface::callStartFileCallback(const std::string& filePath)
{
    std::scoped_lock lock(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::START_FILE, filePath});
}

void CallbackInterface::callEndFileCallback(const std::string& filePath)
{
    std::scoped_lock lock(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::END_FILE, filePath});
}

void CallbackInterface::callStreamPosChangeCallback(size_t streamPos, TimeType timeType)
{
    // Frames are placed first, because they will be called more often than seconds.
    if (timeType == TimeType::FRAMES)
    {
        std::scoped_lock lock(m_callbackCallMutex);
        m_callbackCall.push_back({CallbackType::STREAM_POS_CHANGE_IN_FRAME, streamPos});
    }
    else
    {
        std::scoped_lock lock(m_callbackCallMutex);
        m_callbackCall.push_back({CallbackType::STREAM_POS_CHANGE, streamPos});
    }
}

void CallbackInterface::callStreamPausedCallback()
{
    std::scoped_lock lock(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::STREAM_PAUSED});
}

void CallbackInterface::callStreamPlayingCallback()
{
    std::scoped_lock lock(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::STREAM_PLAYING});
}

void CallbackInterface::callStreamStoppingCallback()
{
    std::scoped_lock lock(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::STREAM_STOPPING});
}

void CallbackInterface::callStreamBufferingCallback()
{
    std::scoped_lock lock(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::STREAM_BUFFERING});
}

void CallbackInterface::callStreamEnoughBufferingCallback()
{
    std::scoped_lock lock(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::STREAM_ENOUGH_BUFFERING});
}

void CallbackInterface::callIsReadyChangedCallback(bool isReady)
{
    std::scoped_lock lack(m_callbackCallMutex);
    m_callbackCall.push_back({CallbackType::IS_READY_CHANCHED, isReady});
}

void CallbackInterface::callback() 
{
    SAL_DEBUG_LOOP_UPDATE("Processing callbacks")

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

    SAL_DEBUG_LOOP_UPDATE("Removing unneeded threads")

    // Remove unneeded threads.
    std::scoped_lock btLock(m_backgroundThreadMutex);
    for (std::vector<std::thread>::const_reverse_iterator crit = m_backgroundThread.crbegin();
         crit != m_backgroundThread.crend();
         crit++)
    {
        if (!crit->joinable())
        {
            m_backgroundThread.erase((crit+1).base());
        }
    }

    SAL_DEBUG_LOOP_UPDATE("Processing callbacks done")
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
        callbackArray.erase(*(crit+1));
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

void CallbackInterface::setIsReadyGetter(std::function<bool()> getter)
{
    m_isReadyGetter = getter;
}
}
