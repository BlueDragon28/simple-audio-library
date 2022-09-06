#ifndef SIMPLE_AUDIO_LIBRARY_SMDEBUGLOG_H
#define SIMPLE_AUDIO_LIBRARY_SMDEBUGLOG_H

#include <string>
#include <memory>
#include <fstream>
#include <mutex>

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
};
}

#endif // SIMPLE_AUDIO_LIBRARY_SMDEBUGLOG_H
