#include "AbstractAudioFile.h"
#include <cstring>

namespace SAL
{
/*
Opening a file *filePath and prepare it
for streaming.
*/
AbstractAudioFile::AbstractAudioFile(const char* filePath) :
    m_filePath(filePath),
    m_isOpen(false),
    m_tmpBuffer(nullptr),
    m_tmpTailPos(0),
    m_tmpWritePos(0),
    m_tmpSizeDataWritten(0),
    m_tmpSize(0)
{}

AbstractAudioFile::AbstractAudioFile(const std::string& filePath) :
    m_filePath(filePath),
    m_isOpen(false),
    m_tmpBuffer(nullptr),
    m_tmpTailPos(0),
    m_tmpWritePos(0),
    m_tmpSizeDataWritten(0),
    m_tmpSize(0)
{}

AbstractAudioFile::~AbstractAudioFile()
{
    delete[] m_tmpBuffer;
}

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

/*
Read data from the file and put it into
the temporaty buffer.
*/
void AbstractAudioFile::readFromFile()
{
    if (!m_isOpen)
        return;
    
    if (m_tmpTailPos == m_tmpSizeDataWritten)
        readDataFromFile();
}

/*
Resize the temporary buffer.
*/
void AbstractAudioFile::resizeTmpBuffer(size_t size)
{
    char* tmpBuffer = new char[size];

    if (m_tmpBuffer)
    {
        size_t bufferSize;
        if (size < m_tmpSize)
            bufferSize = size;
        else
            bufferSize = m_tmpSize;
        memcpy(m_tmpBuffer, tmpBuffer, bufferSize);
        delete m_tmpBuffer;
    }

    m_tmpBuffer = tmpBuffer;
    m_tmpSize = size;
    if (m_tmpTailPos > size)
        m_tmpTailPos = size;
    if (m_tmpSizeDataWritten > size)
        m_tmpSizeDataWritten = size;
    if (m_tmpWritePos > size)
        m_tmpWritePos = size;
}

char* AbstractAudioFile::tmpBufferPtr()
{
    return m_tmpBuffer;
}
}