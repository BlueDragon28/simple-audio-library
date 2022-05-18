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

    /*
    Vector storing user defined callback.
    */

    std::vector<StartFileCallback> m_startFileCallback;
    std::vector<EndFileCallback> m_endFileCallback;
    std::vector<StreamPosChangeInFramesCallback> m_streamPosChangeInFramesCallback;
    std::mutex m_startFileCallbackMutex,
               m_endFileCallbackMutex,
               m_streamPosChangeInFramesMutex;

    /*
    List storing the callback call.
    */
    std::list<CallbackData> m_callbackCall;
    std::mutex m_callbackCallMutex;
};
}

#endif // SIMPLE_AUDIO_LIBRARY_STREAM_CALLBACK_H_