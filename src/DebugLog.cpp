#include "DebugLog.h"
#include <filesystem>

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
    if (!filePath.empty() && filePath != m_filePath && std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath))
    {
        std::ofstream file(filePath);
        if (file.is_open() && file.good())
        {
            std::scoped_lock lock(m_streamMutex);

            // If the file exists and is writable, open the file to stream it.
            m_filePath = filePath;
            m_stream.close();
            m_stream.open(m_filePath);

            return true;
        }
    }

    return false;
}
}
