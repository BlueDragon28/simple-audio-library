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

    // Stream location
    m_streamPos(0),
    m_streamPosInSamples(0),
    m_streamPosInFrames(0)
{}

AbstractAudioFile::AbstractAudioFile(const std::string& filePath) :
    AbstractAudioFile(filePath.c_str())
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
    if (!m_isOpen || !m_tmpBuffer)
        return;
    
    if (m_tmpWritePos < m_tmpMinimumSize)
    {
        if (m_tmpTailPos == m_tmpSizeDataWritten)
        {
            m_tmpTailPos = 0;
            m_tmpWritePos = 0;
            m_tmpSizeDataWritten = 0;
        }
        readDataFromFile();
    }
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

/*
Insert data into the tmp buffer.
*/
void AbstractAudioFile::insertDataInfoTmpBuffer(char* buffer, size_t size)
{
    if (!m_tmpBuffer)
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
    if (m_tmpTailPos == m_tmpSizeDataWritten || !m_tmpBuffer)
        return;
    
    size_t nbWrited = m_ringBuffer.write(m_tmpBuffer+m_tmpTailPos, m_tmpSizeDataWritten-m_tmpTailPos);
    m_tmpTailPos += nbWrited;
}

/*
Sample rate of the stream.
*/
size_t AbstractAudioFile::sampleRate() const
{
    return m_sampleRate;
}

/*
Number of channels of the stream.
*/
int AbstractAudioFile::numChannels() const
{
    return m_numChannels;
}

/*
Number of bytes per sample of the stream.
*/
int AbstractAudioFile::bytesPerSample() const
{
    return m_bytesPerSample;
}

/*
Number of bits per sample of the stream.
*/
int AbstractAudioFile::bitsPerSample() const
{
    return m_bytesPerSample*8;
}

/*
Size in bytes of the audio data.
*/
size_t AbstractAudioFile::streamSizeInBytes() const
{
    return m_sizeStream;
}

/*
Size in samples of the audio data.
*/
size_t AbstractAudioFile::streamSizeInSamples() const
{
    return m_sizeStreamInSamples;
}

/*
Size in frames of the audio data.
*/
size_t AbstractAudioFile::streamSize() const
{
    return m_sizeStreamInFrames;
}

/*
Update sample rate.
*/
void AbstractAudioFile::setSampleRate(size_t sampleRate)
{
    m_sampleRate = sampleRate;
}

/*
Update number of channels
*/
void AbstractAudioFile::setNumChannels(int numChannels)
{
    m_numChannels = numChannels;
    updateStreamSizeInfo();
}

/*
Update bytes per sample.
*/
void AbstractAudioFile::setBytesPerSample(int bytesPerSample)
{
    m_bytesPerSample = bytesPerSample;
    updateStreamSizeInfo();
}

/*
Update size of the stream in bytes.
*/
void AbstractAudioFile::setSizeStream(size_t sizeStream)
{
    m_sizeStream = sizeStream;
    updateStreamSizeInfo();
}

void AbstractAudioFile::updateStreamSizeInfo()
{
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
    if (!m_isOpen || m_ringBuffer.size() == 0 || m_streamPos == m_sizeStream)
        return 0;
    
    size_t sizeInBytes = sizeInFrames * numChannels() * bytesPerSample();
    size_t bytesReaded = m_ringBuffer.read(data, sizeInBytes);
    m_streamPos += bytesReaded;
    updateStreamPosInfo();

    size_t bytesReadedInFrames;
    if (bytesReaded != 0)
        bytesReadedInFrames = bytesReaded / numChannels() / bytesPerSample();
    else
        bytesReadedInFrames = 0;

    return bytesReadedInFrames;
}

/*
Update the buffers size when the 
audio file header is readed.
*/
void AbstractAudioFile::updateBuffersSize()
{
    resizeTmpBuffer(sampleRate() * numChannels() * bytesPerSample());
    m_ringBuffer.resizeBuffer(sampleRate() * numChannels() * bytesPerSample() * 5);
}

void AbstractAudioFile::updateStreamPosInfo()
{
    m_streamPosInSamples = m_streamPos * bytesPerSample();
    m_streamPosInFrames = m_streamPosInSamples * numChannels();
}

/*
Return the position of the stream in frames.
*/
size_t AbstractAudioFile::streamPos() const 
{
    return m_streamPosInFrames;
}

/*
Return the position of the stream in samples.
*/
size_t AbstractAudioFile::streamPosInSamples() const
{
    return m_streamPosInSamples;
}

/*
Return the position of the stream in bytes.
*/
size_t AbstractAudioFile::streamPosInBytes() const
{
    return m_streamPos;
}

/*
Reset stream position.
*/
void AbstractAudioFile::resetStreamPosition()
{
    m_streamPos = 0;
    updateStreamPosInfo();
}
}