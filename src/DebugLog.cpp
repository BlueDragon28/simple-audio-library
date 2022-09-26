#include "DebugLog.h"
#include <filesystem>
#include <ctime>

namespace SAL
{
std::unique_ptr<DebugLog> DebugLog::_instance;

DebugLog::DebugLog()
{}

DebugLog::~DebugLog()
{}

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
    std::scoped_lock lock(m_streamMutex);

    bool isFileExist = std::filesystem::exists(filePath);

    if (!filePath.empty() && filePath != m_filePath && 
        ((isFileExist && std::filesystem::is_regular_file(filePath)) || !isFileExist))
    {
        std::ofstream file(filePath);
        if (file.is_open() && file.good())
        {
            std::scoped_lock lock(m_streamMutex);

            // Close the test file stream.
            file.close();

            // If the file exists and is writable, open the file to stream it.
            m_filePath = filePath;
            m_stream.close();
            m_stream.open(m_filePath);

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
            m_stream << item.toString();
        }

        // Remove every elements in the list.
        m_listItems.clear();
    }
}

std::string DebugLog::DebugOutputItem::toString() const
{
    std::string str;

    // Convert the time into a string.
    std::time_t timeT = std::chrono::system_clock::to_time_t(time);
    str += std::ctime(&timeT);

    // Append class name, function/method name and the message.
    if (!className.empty())
    {
        str += " " + className + "::";
    }
    str += functionName + ": " + msg + ".";

    return str;
}
}
