#ifndef SIMPLE_AUDIO_LIBRARY_SMDEBUGLOG_H
#define SIMPLE_AUDIO_LIBRARY_SMDEBUGLOG_H

#include <string>
#include <memory>
#include <fstream>
#include <mutex>
#include <chrono>
#include <vector>

namespace SAL
{
/*
Singleton class to handle debug output into a file.
*/
class DebugLog
{
    DebugLog();
    DebugLog(const DebugLog& other) = delete;

public:
    ~DebugLog();

    /*
    Instanciate an single instance of the DebugLog class.
    */
    static DebugLog* instance();

    /*
    Set the file path to output the debug informations into.
    */
    bool setFilePath(const std::string& filePath);

    /*
    Append a debug item.
    className: the class name,
    functionName: the function or member name,
    msg: the message to output.
    */
    void append(
            const std::string& className,
            const std::string& functionName,
            std::string msg);

    /*
    Flush data into the file.
    */
    void flush();

private:
    /*
    Instance of the singleton class.
    */
    static std::unique_ptr<DebugLog> _instance;

    /*
    The filepath to output debug informations into.
    */
    std::string m_filePath;

    /*
    Stream to write log text into a file.
    */
    std::ofstream m_stream;

    /*
    Mutex to block the stream to be called simultaneously from different thread.
    */
    std::mutex m_streamMutex;

    /*
    Struct holding informations about a debug output item.
    */
    struct DebugOutputItem
    {
        std::string className;
        std::string functionName;
        std::chrono::system_clock::time_point time;
        std::string msg;

        /*
        Convert debug item into a string.
        */
        std::string toString() const;
    };

    /*
    List of the debug output to flush into the debug file.
    */
    std::vector<DebugOutputItem> m_listItems;
};
}

#endif // SIMPLE_AUDIO_LIBRARY_SMDEBUGLOG_H
