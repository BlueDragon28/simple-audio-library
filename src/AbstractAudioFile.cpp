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
    m_tmpSize(0),
    m_tmpMinimumSize(0),

    // Audio file info
    m_sampleRate(0),
    m_numChannels(0),
    m_bytesPerSample(0),
    m_sizeStream(0),
    m_sizeStreamInSamples(0),
    m_sizeStreamInFrames(0),
    m_sampleType(SampleType::UNKNOWN),

    // Indicate where the data start in the audio file.
    m_startDataPos(0),

    // Stream location
    m_streamPos(0),
    m_streamPosInSamples(0),
    m_streamPosInFrames(0),

    // Indicate no more data need to be readed.
    m_endFile(false),

    // Is the stream has reached the end.
    m_isEnded(false),

    // Streaming pos from audio file.
    m_readPos(0)
{}

AbstractAudioFile::AbstractAudioFile(const std::string& filePath) :
    AbstractAudioFile(filePath.c_str())
{}

AbstractAudioFile::~AbstractAudioFile()
{
    delete[] m_tmpBuffer;
}

/*
Read data from the file and put it into
the temporaty buffer.
*/
void AbstractAudioFile::readFromFile()
{
    std::scoped_lock lock(m_readFromFileMutex);
    if (!m_isOpen || m_endFile)
        return;

    if (m_tmpTailPos == m_tmpSizeDataWritten)
    {
        m_tmpTailPos = 0;
        m_tmpWritePos = 0;
        m_tmpSizeDataWritten = 0;
    }
    
    if (m_tmpWritePos < m_tmpMinimumSize)
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
        memcpy(tmpBuffer, m_tmpBuffer, bufferSize);
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

/*
Insert data into the tmp buffer.
*/
void AbstractAudioFile::insertDataInfoTmpBuffer(char* buffer, size_t size)
{
    if (size == 0)
        return;
    
    if (m_tmpWritePos + size > m_tmpSize)
        resizeTmpBuffer(m_tmpWritePos + size);
    
    memcpy(m_tmpBuffer+m_tmpWritePos, buffer, size);

    m_tmpWritePos += size;
    m_tmpSizeDataWritten += size;
}

/*
Flush the temporary data into the ring buffer.
*/
void AbstractAudioFile::flush()
{
    std::scoped_lock lock(m_readFromFileMutex);
    if (m_tmpTailPos == m_tmpSizeDataWritten || !m_tmpBuffer)
        return;
    
    size_t nbWrited = m_ringBuffer.write(m_tmpBuffer+m_tmpTailPos, m_tmpSizeDataWritten-m_tmpTailPos);
    m_tmpTailPos += nbWrited;
}

void AbstractAudioFile::updateStreamSizeInfo()
{
    if (m_sizeStream == 0 || m_bytesPerSample == 0 || m_numChannels == 0)
        return;
    m_sizeStreamInSamples = m_sizeStream / m_bytesPerSample;
    m_sizeStreamInFrames = m_sizeStream / m_bytesPerSample / m_numChannels;
}

/*
Extract data from the audio files.
- data = a pointer to a sound buffer.
- sizeInFrames = the size of the buffers in frames.
- return the size of data read in frames.
*/
size_t AbstractAudioFile::read(char* data, size_t sizeInFrames)
{
    if (!m_isOpen || m_ringBuffer.size() == 0 || m_isEnded)
        return 0;
    std::scoped_lock lock(m_seekMutex);
    
    size_t sizeInBytes = sizeInFrames * numChannels() * bytesPerSample();
    size_t bytesReaded = m_ringBuffer.read(data, sizeInBytes);
    m_streamPos += bytesReaded;
    updateStreamPosInfo();

    if (m_streamPos == m_sizeStream)
        m_isEnded = true;

    size_t bytesReadedInFrames;
    if (bytesReaded != 0)
        bytesReadedInFrames = bytesReaded / numChannels() / bytesPerSample();
    else
    {
        // If no data to read and the file reached the end, stop the stream.
        if (m_endFile)
            m_isEnded = true;
        bytesReadedInFrames = 0;
    }

    return bytesReadedInFrames;
}

/*
Update the buffers size when the 
audio file header is readed.
*/
void AbstractAudioFile::updateBuffersSize()
{
    resizeTmpBuffer(sampleRate() * numChannels() * bytesPerSample());
    m_tmpMinimumSize = m_tmpSize;
    m_ringBuffer.resizeBuffer(sampleRate() * numChannels() * bytesPerSample() * 5);
}

void AbstractAudioFile::updateStreamPosInfo()
{
    m_streamPosInSamples = m_streamPos / bytesPerSample();
    m_streamPosInFrames = m_streamPosInSamples / numChannels();
}

/*
Increment the position of the reading position
of the audio data and set endFile to true when the data
has reach the end of the file.
*/
void AbstractAudioFile::incrementReadPos(size_t size)
{
    if (size == 0)
        return;
    
    m_readPos += size;

    if (m_readPos >= streamSizeInBytes())
        endFile();
}

/*
Seeking a position (in frames) in the audio stream.
This will clear all the buffers and start playing
at the position needed if the position if valid.
*/
void AbstractAudioFile::seek(size_t pos)
{
    std::scoped_lock lock(m_seekMutex);
    if (pos < streamSize())
    {
        m_ringBuffer.clear();
        if (updateReadingPos(pos))
        {
            m_readPos = pos * bytesPerSample() * numChannels();
            m_streamPos = m_readPos;
            updateStreamPosInfo();
            m_tmpTailPos = 0L;
            m_tmpWritePos = 0L;
            m_tmpSizeDataWritten = 0L;
            m_endFile = false;
            m_isEnded = false;
        }
    }
}
}