#include "WaveAudioFile.h"
#include "DebugLog.h"
#include <cstring>
#include <vector>

// Redefine CLASS_NAME to have the name of the class.
#undef CLASS_NAME
#define CLASS_NAME "WaveAudioFile"

namespace SAL
{
/*
Opening a file *filePath and prepare it
for streaming.
*/
WaveAudioFile::WaveAudioFile(const std::string& filePath) :
    AbstractAudioFile(filePath)
{
    open();
}

WaveAudioFile::~WaveAudioFile()
{
    close();
}

void WaveAudioFile::open()
{
#ifndef NDEBUG
    SAL_DEBUG("Opening file " + filePath());
#endif

    if (filePath().empty())
        return;
    
    m_audioFile.open(filePath(), std::fstream::binary);
    if (m_audioFile.is_open())
    {
        // RIFF identifier.
        char RIFF[5] = {0, 0, 0, 0, 0};
        m_audioFile.read(RIFF, 4);
        if (strcmp(RIFF, "RIFF") != 0 || m_audioFile.fail())
            return;
        
        unsigned int fileSize = 0;
        m_audioFile.read((char*)&fileSize, 4);
        if (fileSize == 0 || m_audioFile.fail())
            return;
        
        // WAVE identifier.
        char WAVE[5] = {0, 0, 0, 0, 0};
        m_audioFile.read(WAVE, 4);
        if (strcmp(WAVE, "WAVE") != 0 || m_audioFile.fail())
            return;
        
        // "fmt " identifier.
        char fmt_[5] = {0, 0, 0, 0, 0};
        m_audioFile.read(fmt_, 4);
        if (strcmp(fmt_, "fmt ") != 0 || m_audioFile.fail())
            return;
        
        // "fmt " size.
        int fmt_size = 0;
        m_audioFile.read((char*)&fmt_size, 4);
        if (fmt_size != 16 && fmt_size != 18 
            && fmt_size != 40 || m_audioFile.fail())
            return;
        
        // PCM format
        unsigned short pcmFormat = 0;
        m_audioFile.read((char*)&pcmFormat, 2);
        if ((pcmFormat != 1 && pcmFormat != 65534) || m_audioFile.fail())
            return;
        
        // Channels
        short channels = 0;
        m_audioFile.read((char*)&channels, 2);
        if (channels < 1 && channels > 6 || m_audioFile.fail())
            return;
        
        // Sample rate
        int sampleRate = 0;
        m_audioFile.read((char*)&sampleRate, 4);
        if (sampleRate <= 0 || m_audioFile.fail())
            return;
        
        // Passing 4 bytes
        int dummyInteger;
        m_audioFile.read((char*)&dummyInteger, 4);
        if (m_audioFile.fail())
            return;
        
        // Passing 2 bytes.
        m_audioFile.read((char*)&dummyInteger, 2);
        if (m_audioFile.fail())
            return;
        
        // Bits per sample
        short bitsPerSample = 0;
        m_audioFile.read((char*)&bitsPerSample, 2);
        if (bitsPerSample != 8 && bitsPerSample != 16 && 
            bitsPerSample != 24 && bitsPerSample != 32 ||
            m_audioFile.fail())
            return;
        
        // Read extra bytes.
        if (fmt_size == 18 || fmt_size == 40)
        {
            std::vector<char> extraBytes(fmt_size-16);
            m_audioFile.read(extraBytes.data(), fmt_size-16);
            if (m_audioFile.fail())
                return;
        }

        // Next identifier
        char nextIdentifier[5] = {0, 0, 0, 0, 0};
        m_audioFile.read(nextIdentifier, 4);
        if (m_audioFile.fail())
            return;
        
        bool isFloatStream = false;

        // Read "fact" section if available.
        if (strcmp(nextIdentifier, "fact") == 0)
        {
            // "fact" size.
            int factSize = 0;
            m_audioFile.read((char*)&factSize, 4);
            if (factSize <= 0 || m_audioFile.fail())
                return;
            
            // Reading the fact section without processing it.
            std::vector<char> factData(factSize);
            memset(factData.data(), 0, factSize);
            m_audioFile.read(factData.data(), factSize);
            if (m_audioFile.fail())
                return;
            
            // The "fact" section mean the wave file is a float stream.
            isFloatStream = true;
            
            // Read the identifier of the next section.
            m_audioFile.read(nextIdentifier, 4);
            if (m_audioFile.fail())
                return;
        }
        
        // If next identifier is "LIST" identifier.
        if (strcmp(nextIdentifier, "LIST") == 0)
        {
            // "LIST" size.
            int listSize = 0;
            m_audioFile.read((char*)&listSize, 4);
            if (listSize <= 0 || m_audioFile.fail())
                return;
            
            // Read track name.
            std::vector<char> trackName(listSize+1);
            memset(trackName.data(), 0, listSize+1);
            m_audioFile.read(trackName.data(), listSize);
            if (m_audioFile.fail())
                return;
            
            // Read "data" identifier into nextIdentifier.
            m_audioFile.read(nextIdentifier, 4);
            if (m_audioFile.fail())
                return;
        }
        else
            return;

        // "data" identifier.
        if (strcmp(nextIdentifier, "data") != 0 || m_audioFile.fail())
            return;
        
        // Audio file size in bytes.
        unsigned int audioDataSize = 0;
        m_audioFile.read((char*)&audioDataSize, 4);
        if (audioDataSize == 0 || audioDataSize > fileSize || m_audioFile.fail())
            return;
        
        // PCM format
        SampleType pcmFormatType;
        if (!isFloatStream)
        {
            if (bitsPerSample > 8)
                pcmFormatType = SampleType::INT;
            else
                pcmFormatType = SampleType::UINT;
        }
        else if (bitsPerSample == 32)
            pcmFormatType = SampleType::FLOAT;
        else
            return;
        
        // Then the data will be streamed.

        // Store headers data.
        setNumChannels(channels);
        setSampleRate(sampleRate);
        setBytesPerSample(bitsPerSample/8);
        setSizeStream(audioDataSize);
        setDataStartingPoint(m_audioFile.tellg());
        updateBuffersSize();
        setSampleType(pcmFormatType);
        fileOpened();
    }

    SAL_DEBUG("Opening file done")
}

void WaveAudioFile::close()
{
    if (m_audioFile.is_open())
        m_audioFile.close();
}

void WaveAudioFile::readDataFromFile()
{
    // Check if the file is open and there is data to read.
    if (!m_audioFile.is_open() || streamSizeInBytes() == 0)
        return;

    SAL_DEBUG("Reading data from file")
    
    // Get the size of the tmp buffer.
    size_t readSize = minimumSizeTemporaryBuffer();
    if (readPos() + readSize > streamSizeInBytes())
        readSize = streamSizeInBytes() - readPos();
    
    // Get data from file.
    std::vector<char> data(readSize);
    memset(data.data(), 0, readSize);

    m_audioFile.read(data.data(), readSize);

    if (m_audioFile.fail())
    {
        endFile();
        return;
    }

    // Send data into the tmp buffer.
    insertDataInfoTmpBuffer(data.data(), readSize);
    incrementReadPos(readSize);

    SAL_DEBUG("Reading data from file done")
}

bool WaveAudioFile::updateReadingPos(size_t pos)
{
    SAL_DEBUG("Update reading position")

    // Pos in bytes.
    pos *= bytesPerSample() * numChannels();
    // Headers offset.
    pos += dataStartingPoint();

    // Updating ifstream position.
    m_audioFile.seekg(pos);
    if (!m_audioFile.fail())
        return true;
    else
        return false;
}
}