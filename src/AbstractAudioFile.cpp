#include "AbstractAudioFile.h"

namespace SAL
{
/*
Opening a file *filePath and prepare it
for streaming.
*/
AbstractAudioFile::AbstractAudioFile(const char* filePath) :
    m_filePath(filePath),
    m_isOpen(false)
{}

AbstractAudioFile::AbstractAudioFile(const std::string& filePath) :
    m_filePath(filePath),
    m_isOpen(false)
{}

AbstractAudioFile::~AbstractAudioFile()
{}

/*
Return the file path of the audio file.
*/
const std::string& AbstractAudioFile::filePath() const
{
    return m_filePath;
}

bool AbstractAudioFile::isOpen() const
{
    return m_isOpen;
}

/*
Mark the file has ready to stream.
*/
void AbstractAudioFile::fileOpened(bool value)
{
    m_isOpen = value;
}
}