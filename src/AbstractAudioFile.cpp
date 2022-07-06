#include "AbstractAudioFile.h"
#include <cstring>
#include <limits>
#include <vector>

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

// Template to convert data from an integer to a float number.
template<typename T>
std::vector<float> intArrayToFloatArray(T* iBuffer, size_t samples)
{
    std::vector<float> fBuffer(samples);

    for (size_t i = 0; i < samples; i++)
    {
        uint32_t max = 0;
        uint32_t min = 0;

        // Get max and min value based on the type.
        if (sizeof(T) == 1)
        {
            max = 0x7F; // 127
            min = 0x80; // 128 : the max negative number is always one number higher.
        }
        else if (sizeof(T) == 2)
        {
            max = 0x7FFF;
            min = 0x8000;
        }
        else if (sizeof(T) == 4)
        {
            max = 0x7FFFFFFF;
            min = 0x80000000;
        }

        // Convert int to float and in the range [-1,1].
        float number = (float)iBuffer[i];
        fBuffer[i] = number / (number < 0 ? (float)min : (float)max);
    }

    return fBuffer;
}

// Convert 24 bits integers to floating point numbers.
// Since no 24 bits integer exist in c++, this fonction convert manually the number to a 32 bit integers.
template<>
std::vector<float> intArrayToFloatArray(FakeInt24* iBuffer, size_t samples)
{
    std::vector<float> fBuffer(samples);

    for (size_t i = 0; i < samples; i++)
    {
        // Min and max value.
        uint32_t max = 0x7FFFFF;
        uint32_t min = 0x800000;

        FakeInt24 number24 = iBuffer[i];
        int number32 = 0;

        // Check if the number have the negative sign.
        if (number24.c[2] & 0x80)
        {
            number32 = 0xFFFFFFFF; // When negative all the bits are flipped. 0xFFFFFFFF is equal to -1.
        }

        // Copy number24 to number32.
        FakeInt24* n32 = reinterpret_cast<FakeInt24*>(&number32);
        n32->c[0] = number24.c[0];
        n32->c[1] = number24.c[1];
        n32->c[2] = number24.c[2]; // No need to worry about the 8 bit because if it's negative, it will be equal to 1.

        // Convert number32 to float and in the range [-1,1]
        float fNumber = (float)number32;
        fBuffer[i] = fNumber / (fNumber < 0 ? (float)min : (float)max);
    }

    return fBuffer;
}

/*
Insert data into the tmp buffer.
The data is converted to 32 bits floating point number.
*/
void AbstractAudioFile::insertDataInfoTmpBuffer(char* buffer, size_t size)
{
    if (size == 0)
        return;

    std::vector<float> data;

    // Convert the input stream to 32 bits floating point numbers and copy it into the tmp buffer.
    // Signed integers to floating point numbers.
    if (m_sampleType == SampleType::INT)
    {
        if (bytesPerSample() == 1)
        {
            data = intArrayToFloatArray<char>(buffer, size);
        }
        else if (bytesPerSample() == 2)
        {
            data = intArrayToFloatArray<short>((short*)buffer, size/bytesPerSample());
        }
        else if (bytesPerSample() == 3)
        {
            data = intArrayToFloatArray<FakeInt24>((FakeInt24*)buffer, size/bytesPerSample());
        }
        else if (bytesPerSample() == 4)
        {
            data = intArrayToFloatArray<int>((int*)buffer, size/bytesPerSample());
        }
    }
    // For float, just copy the data.
    else if (m_sampleType == SampleType::FLOAT)
    {
        data.resize(size/bytesPerSample());
        memcpy(data.data(), buffer, size);
    }
    // Unsigned 8 bit integer to floating point number.
    else
    {
        /*
        To convert a unsigned integer into a floating point number, the number is divided by
        half the max value of the integer (127.5 here) to get a range between [0,2] and then subtracted by 1
        to get a range between [-1,1].
        */
        float max = 127.5f;
        data.resize(size);
        uint8_t* b = (uint8_t*)buffer;

        for (size_t i = 0; i < size; i++)
        {
            data[i] = (float)b[i] / max;
            data[i] -= 1.0f;
        }
    }

    // Copy data to the tmp buffer.
    size_t sizeDataInBytes = data.size() * sizeof(float);
    if (m_tmpWritePos + sizeDataInBytes > m_tmpSize)
        resizeTmpBuffer(m_tmpWritePos + sizeDataInBytes);
    memcpy(m_tmpBuffer+m_tmpWritePos, data.data(), data.size() * sizeof(float));

    m_tmpWritePos += sizeDataInBytes;
    m_tmpSizeDataWritten += sizeDataInBytes;
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
    m_bytesPerFrame = m_bytesPerSample * m_numChannels;
    m_sizeStreamInSamples = m_sizeStream / m_bytesPerSample;
    m_sizeStreamInFrames = m_sizeStream / m_bytesPerSample / m_numChannels;
}

/*
Extract data from the audio files has 32 bits floating point numbers.
- data = a pointer to a sound buffer.
- sizeInFrames = the size of the buffers in frames.
- return the size of data read in frames.
*/
size_t AbstractAudioFile::read(char* data, size_t sizeInFrames)
{
    if (!m_isOpen || m_ringBuffer.size() == 0 || m_isEnded)
        return 0;
    std::scoped_lock lock(m_seekMutex);
    
    size_t sizeInBytes = sizeInFrames * numChannels() * sizeof(float);
    size_t bytesReaded = m_ringBuffer.read(data, sizeInBytes);
    m_streamPos += bytesReaded / sizeof(float) * m_bytesPerSample;
    updateStreamPosInfo();

    if (m_streamPos == m_sizeStream)
        m_isEnded = true;

    size_t bytesReadedInFrames;
    if (bytesReaded != 0)
        bytesReadedInFrames = bytesReaded / numChannels() / sizeof(float);
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
    resizeTmpBuffer(sampleRate() * numChannels() * sizeof(float));
    m_tmpMinimumSize = m_tmpSize;
    m_ringBuffer.resizeBuffer(sampleRate() * numChannels() * sizeof(float) * 5);
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
Seeking a position (in frames) in the raw stream.
This will clear all the buffers and start playing at 
the position needed if the position if valid.
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