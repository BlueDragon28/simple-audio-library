#ifndef SIMPLE_AUDIO_LIBRARY_STREAM_CALLBACK_H_
#define SIMPLE_AUDIO_LIBRARY_STREAM_CALLBACK_H_

#include <string>
#include <functional>
#include <vector>
#include <variant>
#include <list>
#include <mutex>

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
    };

    typedef std::variant<std::monostate,
                        size_t,
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
    Add a stream position (in frames) change callback.
    This callback is called when the position of the 
    stream is changing.
    */
    void addStreamPosChangeInFramesCallback(StreamPosChangeInFramesCallback callback);

    /*
    Add a stream position (in seconds) change callback.
    This callback is called when the position of the stream
    is changing.
    */
    void addStreamPosChangeCallback(StreamPosChangeCallback callback);

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
    Calling stream position (in frames) change callback.
    */
    void callStreamPosChangeInFramesCallback(size_t streamPos);

    /*
    Calling stream position (in seconds) change callback.
    */
    void callStreamPosChangeCallback(size_t streamPos);

    /*
    Calling stream paused callback.
    */
    void callStreamPausedCallback();

    /*
    Calling stream playing callback
    */
    void callStreamPlayingCallback();

    /*
    Call every callback inside the callback queue.
    */
    void callback();

private:
    /*
    Calling the callback of every type of callback.
    */

    void startFileCallback(const std::string& filePath);
    void endFileCallback(const std::string& filePath);
    void streamPosChangeInFramesCallback(size_t streamPos);
    void streamPosChangeCallback(size_t streamPos);
    void streamPausedCallback();
    void streamPlayingCallback();

    /*
    Vector storing user defined callback.
    */

    std::vector<StartFileCallback> m_startFileCallback;
    std::vector<EndFileCallback> m_endFileCallback;
    std::vector<StreamPosChangeInFramesCallback> m_streamPosChangeInFramesCallback;
    std::vector<StreamPosChangeCallback> m_streamPosChangeCallback;
    std::vector<StreamPausedCallback> m_streamPausedCallback;
    std::vector<StreamPlayingCallback> m_streamPlayingCallback;
    std::mutex m_startFileCallbackMutex,
               m_endFileCallbackMutex,
               m_streamPosChangeInFramesMutex,
               m_streamPosChangeMutex,
               m_streamPausedMutex,
               m_streamPlayingMutex;

    /*
    List storing the callback call.
    */
    std::list<CallbackData> m_callbackCall;
    std::mutex m_callbackCallMutex;
};
}

#endif // SIMPLE_AUDIO_LIBRARY_STREAM_CALLBACK_H_