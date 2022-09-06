#ifndef SIMPLE_AUDIO_LIBRARY_SMDEBUGLOG_H
#define SIMPLE_AUDIO_LIBRARY_SMDEBUGLOG_H

#include <string>
#include <memory>

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

private:
    /*
    Instance of the singleton class.
    */
    static std::unique_ptr<DebugLog> _instance;
};
}

#endif // SIMPLE_AUDIO_LIBRARY_SMDEBUGLOG_H
