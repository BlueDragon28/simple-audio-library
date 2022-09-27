#include "DebugLog.h"
#include <filesystem>
#include <ctime>

namespace SAL
{
std::unique_ptr<DebugLog> DebugLog::_instance;

DebugLog::DebugLog() :
    m_isRunning(false)
{}

DebugLog::~DebugLog()
{
    destroyUpdateThread();
}

DebugLog* DebugLog::instance()
{
    if (!_instance)
    {
        _instance = std::unique_ptr<DebugLog>(new DebugLog());
    }

    return _instance.get();
}

bool DebugLog::setFilePath(const std::string &filePath)
{
    // Stop the update thread.
    destroyUpdateThread();

    std::scoped_lock lock(m_streamMutex);

    bool isFileExist = std::filesystem::exists(filePath);

    if (!filePath.empty() && filePath != m_filePath && 
        ((isFileExist && std::filesystem::is_regular_file(filePath)) || !isFileExist))
    {
        // If the file do not exist, try to create the directory containing it to be sure.
        if (!isFileExist)
        {
            createFolder(filePath);
        }

        std::ofstream file(filePath);
        if (file.is_open() && file.good())
        {
            // Close the test file stream.
            file.close();

            // If the file exists and is writable, open the file to stream it.
            m_filePath = filePath;
            m_stream.close();
            m_stream.open(m_filePath);

            // Creating the update thread to write the logs
            createUpdateThread();

            return true;
        }
    }

    return false;
}

void DebugLog::append(const std::string &className, const std::string &functionName, std::string msg)
{
    std::scoped_lock lock(m_streamMutex);

    if (!functionName.empty() && !msg.empty())
    {
        // Pushing the debug item into the list.
        m_listItems.push_back({
              className,
              functionName,
              std::chrono::system_clock::now(),
              msg
          });
    }
}

void DebugLog::flush()
{
    std::scoped_lock lock(m_streamMutex);

    // If there is a file opened, flush the data into it.
    if (m_stream.is_open())
    {
        // Serialize every item in the debug listItems list.
        for (const DebugOutputItem& item : m_listItems)
        {
            // Convert the item into a string and send it into the file.
            m_stream << item.toString() + '\n';
        }

        // Remove every elements in the list.
        m_listItems.clear();
    }
}

void DebugLog::update()
{
    // Using to know when to flush
    int counter = 0;

    while (m_isRunning)
    {
        // Flush the logs once every seconds.
        if (counter % 3000 == 0)
        {
            flush();
            counter = 0;
            continue;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        counter += 50;
    }
}

void DebugLog::createUpdateThread()
{
    // First destroying the existing thread.
    destroyUpdateThread();

    // Creating the new thread
    m_isRunning = true;
    m_flushThread = std::thread(&DebugLog::update, this);
}

void DebugLog::destroyUpdateThread()
{
    // Check if the thread is running
    if (m_isRunning && m_flushThread.joinable())
    {
        // Setting isRunning to false to stop the loop and wait until it stop.
        m_isRunning = false;
        m_flushThread.join();

        // Then flush, to be sure that no log is lost.
        flush();
    }
}

std::string DebugLog::DebugOutputItem::toString() const
{
    std::string str;

    // Convert the time into a string.
    std::time_t timeT = std::chrono::system_clock::to_time_t(time);
    const std::string timeStr = std::ctime(&timeT);
    if (!timeStr.empty())
    {
        str.append(timeStr.cbegin(), timeStr.cend()-1);
    }

    // Append class name, function/method name and the message.
    if (!className.empty())
    {
        str += " " + className + "::";
    }
    str += functionName + ": " + msg + ".";

    return str;
}

std::string DebugLog::getFolderPart(const std::string& filePath) const
{
    if (!filePath.empty() && !std::filesystem::is_directory(filePath))
    {
        // Retrieving the part of the directory location.
        size_t dashPos = filePath.find_last_of('/');

#ifdef WIN32
        // If it fail, check using the windows backdash instead.
        if (dashPos == std::string::npos)
        {
            dashPos == filePath.find_last_of('\\');
        }
#endif

        // If the last occurent of a dash has been found, retrieve the string to this dash.
        if (dashPos != std::string::npos)
        {
            return filePath.substr(0, dashPos);
        }
    }
    
    // No valid directory path found.
    return "";
}

bool DebugLog::createFolder(const std::string& filePath) const
{
    if (!filePath.empty())
    {
        // Retrieve the directory part of the file.
        const std::string dirPath = getFolderPart(filePath);

        // If there is a directory part and it does not exist, create it.
        if (!dirPath.empty() && !std::filesystem::exists(dirPath))
        {
            std::filesystem::create_directory(dirPath);
        }

        if (std::filesystem::is_directory(dirPath))
        {
            return true;
        }
    }

    return false;
}
}
