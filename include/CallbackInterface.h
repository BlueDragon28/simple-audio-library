#ifndef SIMPLE_AUDIO_LIBRARY_STREAM_CALLBACK_H_
#define SIMPLE_AUDIO_LIBRARY_STREAM_CALLBACK_H_

#include "Common.h"
#include <string>
#include <functional>
#include <vector>
#include <variant>
#include <list>
#include <mutex>
#include <thread>

namespace SAL
{
/*
This class store the callbacks to notify the user
of any change in the stream.
*/
class CallbackInterface
{
    enum class CallbackType
    {
        UNKNOWN,
        START_FILE,
        END_FILE,
        STREAM_POS_CHANGE_IN_FRAME,
        STREAM_POS_CHANGE,
        STREAM_PAUSED,
        STREAM_PLAYING,
        STREAM_STOPPING,
        STREAM_BUFFERING,
        STREAM_ENOUGH_BUFFERING,
        IS_READY_CHANCHED,
    };

    typedef std::variant<std::monostate,
                        size_t,
                        bool,
                        std::string> CallbackCallVariant;
    
    struct CallbackData
    {
        CallbackType type;
        CallbackCallVariant data;
    };

public:
    typedef std::function<void(const std::string&)> EndFileCallback;
    typedef EndFileCallback StartFileCallback;
    typedef std::function<void(size_t)> StreamPosChangeInFramesCallback;
    typedef StreamPosChangeInFramesCallback StreamPosChangeCallback;
    typedef std::function<void()> StreamPausedCallback;
    typedef StreamPausedCallback StreamPlayingCallback;
    typedef StreamPausedCallback StreamStoppingCallback;
    typedef StreamPausedCallback StreamBufferingCallback;
    typedef StreamPausedCallback StreamEnoughBufferingCallback;
    typedef std::function<void(bool)> IsReadyChangedCallback;

    CallbackInterface();
    ~CallbackInterface();

    /*
    Add a start file callback to the list of callback.
    This callback if called when a (new) file start to play.
    */
    void addStartFileCallback(StartFileCallback callback);

    /*
    Add a end file callback to the list of callback.
    This callback is called when a file reach end stream.
    */
    void addEndFileCallback(EndFileCallback callback);

    /*
    Add a stream position (in frames or seconds) change callback.
    This callback is called when the position of the stream
    is changing.
    */
    void addStreamPosChangeCallback(StreamPosChangeCallback callback, TimeType timeType = TimeType::SECONDS);

    /*
    Add a stream paused callback.
    This callback is called when the stream is paused.
    */
    void addStreamPausedCallback(StreamPausedCallback callback);

    /*
    Add a stream playing callback.
    This callback is called when the stream start playing or is resuming.
    */
    void addStreamPlayingCallback(StreamPlayingCallback callback);

    /*
    Add a stream stopping callback.
    This callback is called when a stream stop playing.
    */
    void addStreamStoppingCallback(StreamStoppingCallback callback);

    /*
    Add a stream buffering callback.
    This callback is called when the stream is buffering.
    */
    void addStreamBufferingCallback(StreamBufferingCallback callback);

    /*
    Add a stream enough buffering callback.
    This callback is called when the stream have finish buffering
    */
    void addStreamEnoughBufferingCallback(StreamEnoughBufferingCallback callback);

    /*
    Add is ready changed callback.
    This callback is called whenever the isReady is called.
    */
    void addIsReadyChangedCallback(IsReadyChangedCallback callback);

private:
    /*
    Calling start file callback.
    This event is store inside a list and is then call
    from the main loop of the AudioPlayer class.
    */
    void callStartFileCallback(const std::string& filePath);

    /*
    Calling end file callback.
    This event is store inside a list and is then call
    from the main loop of the AudioPlayer class.
    */
    void callEndFileCallback(const std::string& filePath);

    /*
    Calling stream position (in frames or seconds) change callback.
    */
    void callStreamPosChangeCallback(size_t streamPos, TimeType timeType = TimeType::SECONDS);

    /*
    Calling stream paused callback.
    */
    void callStreamPausedCallback();

    /*
    Calling stream playing callback
    */
    void callStreamPlayingCallback();

    /*
    Calling stream stopping callback.
    */
    void callStreamStoppingCallback();

    /*
    Calling stream buffering callback.
    */
    void callStreamBufferingCallback();

    /*
    Calling stream enough buffering callback.
    */
    void callStreamEnoughBufferingCallback();

    /*
    Calling the is ready changed callback.
    */
    void callIsReadyChangedCallback(bool isReady);

    /*
    Call every callback inside the callback queue.
    */
    void callback();

    /*
    Calling the callback of every type of callback.
    */

    void startFileCallback(const std::string& filePath);
    void endFileCallback(const std::string& filePath);
    void streamPosChangeInFramesCallback(size_t streamPos);
    void streamPosChangeCallback(size_t streamPos);
    void streamPausedCallback();
    void streamPlayingCallback();
    void streamStoppingCallback();
    void streamBufferingCallback();
    void streamEnoughBufferingCallback();
    void isReadyChangedCallback(bool isReady);

    /*
    Set the is ready getter.
    */
    void setIsReadyGetter(std::function<bool()> getter);

    // Call is ready callback thread instance.
    std::vector<std::thread> m_backgroundThread;
    std::mutex m_backgroundThreadMutex;

    // Making the AudioPlayer and Player class a friend of this class.
    friend class AudioPlayer;
    friend class Player;

    /*
    Vector storing user defined callback.
    */

    std::vector<StartFileCallback> m_startFileCallback;
    std::vector<EndFileCallback> m_endFileCallback;
    std::vector<StreamPosChangeInFramesCallback> m_streamPosChangeInFramesCallback;
    std::vector<StreamPosChangeCallback> m_streamPosChangeCallback;
    std::vector<StreamPausedCallback> m_streamPausedCallback;
    std::vector<StreamPlayingCallback> m_streamPlayingCallback;
    std::vector<StreamStoppingCallback> m_streamStoppingCallback;
    std::vector<StreamBufferingCallback> m_streamBufferingCallback;
    std::vector<StreamEnoughBufferingCallback> m_streamEnoughBufferingCallback;
    std::vector<IsReadyChangedCallback> m_isReadyChangedCallback;
    std::mutex m_startFileCallbackMutex,
               m_endFileCallbackMutex,
               m_streamPosChangeInFramesMutex,
               m_streamPosChangeMutex,
               m_streamPausedMutex,
               m_streamPlayingMutex,
               m_streamStoppingMutex,
               m_streamBufferingMutex,
               m_streamEnoughBufferingMutex,
               m_isReadyChangedMutex;

    /*
    List storing the callback call.
    */
    std::list<CallbackData> m_callbackCall;
    std::mutex m_callbackCallMutex;

    // This member variable store the last state of the isReady getter.
    bool m_isReadyLastStatus;
    // Allow this interface to get access to the isReady getter of AudioPlayer.
    std::function<bool()> m_isReadyGetter;
};
}

#endif // SIMPLE_AUDIO_LIBRARY_STREAM_CALLBACK_H_