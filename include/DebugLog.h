#ifndef SIMPLE_AUDIO_LIBRARY_SMDEBUGLOG_H
#define SIMPLE_AUDIO_LIBRARY_SMDEBUGLOG_H

#include "config.h"
#include <string>
#include <memory>
#include <fstream>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>

// Macro to help write debug message.
#ifdef DEBUG_LOG
#define SAL_DEBUG(msg) \
    DebugLog::instance()->append(CLASS_NAME, __func__, msg);
#else
#define SAL_DEBUG(msg)
#endif

// Only available when the option LOG_READ_STREAM is set
#if defined(DEBUG_LOG) && defined(LOG_READ_STREAM)
#define SAL_DEBUG_READ_STREAM(msg) \
    SAL_DEBUG(msg)
#else
#define SAL_DEBUG_READ_STREAM(msg)
#endif

// Only available when the option LOG_READ_FILE is set
#if defined(DEBUG_LOG) && defined(LOG_READ_FILE)
#define SAL_DEBUG_READ_FILE(msg) \
    SAL_DEBUG(msg)
#else
#define SAL_DEBUG_READ_FILE(msg)
#endif

// Only available when the option LOG_OPEN_FILE is set
#if defined(DEBUG_LOG) && defined(LOG_OPEN_FILE)
#define SAL_DEBUG_OPEN_FILE(msg) \
    SAL_DEBUG(msg)
#else
#define SAL_DEBUG_OPEN_FILE(msg)
#endif

// Only available when the option LOG_LOOP_UPDATE is set
#if defined(DEBUG_LOG) && defined(LOG_LOOP_UPDATE)
#define SAL_DEBUG_LOOP_UPDATE(msg) \
    SAL_DEBUG(msg)
#else
#define SAL_DEBUG_LOOP_UPDATE(msg)
#endif

// Only available when the option LOG_STREAM_STATUS is set
#if defined(DEBUG_LOG) && defined(LOG_STREAM_STATUS)
#define SAL_DEBUG_STREAM_STATUS(msg) \
    SAL_DEBUG(msg)
#else
#define SAL_DEBUG_STREAM_STATUS(msg)
#endif

// Only available when the option LOG_SAL_INIT is set
#if defined(DEBUG_LOG) && defined(LOG_SAL_INIT)
#define SAL_DEBUG_SAL_INIT(msg) \
    SAL_DEBUG(msg)
#else
#define SAL_DEBUG_SAL_INIT(msg)
#endif

// Only availabel when the option LOG_EVENTS is set
#if defined (DEBUG_LOG) && defined (LOG_EVENTS)
#define SAL_DEBUG_EVENTS(msg) \
    SAL_DEBUG(msg)
#else
#define SAL_DEBUG_EVENTS(msg)
#endif

#ifdef DEBUG_LOG
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
    The path must be a valid UTF-8 string.
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

private:
    /*
    Flush data into the file.
    */
    void flush();

    /*
    Loop running in a separate thread flushing log every 3 seconds.
    */
    void update();

    /*
    Creating update thread
    */
    void createUpdateThread();

    /*
    Destroying update thread
    */
    void destroyUpdateThread();

    /*
    Getting the folder part of a file path.
    */
    std::string getFolderPart(const std::string& filePath) const;

    /*
    Create the folders of the log if not existing.
    */
    bool createFolder(const std::string& filePath) const;

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

    /*
    The log flushing method running in a separate thread.
    */
    std::thread m_flushThread;

    /*
    The update loop is running until this variable is false
    */
    bool m_isRunning;
};
}
#endif

#endif // SIMPLE_AUDIO_LIBRARY_SMDEBUGLOG_H
