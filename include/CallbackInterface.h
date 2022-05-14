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
        END_FILE
    };

    typedef std::variant<std::monostate,
                        std::string> CallbackCallVariant;
    
    struct CallbackData
    {
        CallbackType type;
        CallbackCallVariant data;
    };

public:
    typedef std::function<void(const std::string&)> EndFileCallback;
    typedef EndFileCallback StartFileCallback;

    CallbackInterface();
    ~CallbackInterface();

    /*
    Add a start file callback to the list of callback.
    This callback if called when a (new) file start to play.
    */
    void addStartFileCallback(StartFileCallback callback);

    /*
    Add a end file callback to the list of callback.
    This callback if called when a (new) file start to play.
    */
    void addEndFileCallbacl(EndFileCallback callback);

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

private:
    /*
    Vector storing user defined callback.
    */

    std::vector<StartFileCallback> m_startFileCallback;
    std::vector<EndFileCallback> m_endFileCallback;
    std::mutex m_startFileCallbackMutex,
               m_endFileCallbackMutex;

    /*
    List storing the callback call.
    */
    std::list<CallbackData> m_callbackCall;
    std::mutex m_callbackCallMutex;
};
}

#endif // SIMPLE_AUDIO_LIBRARY_STREAM_CALLBACK_H_