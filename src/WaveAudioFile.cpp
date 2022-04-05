#include "WaveAudioFile.h"
#include <cstring>

namespace SAL
{
/*
Opening a file *filePath and prepare it
for streaming.
*/
WaveAudioFile::WaveAudioFile(const char* filePath) :
    AbstractAudioFile(filePath)
{
    open();
}

WaveAudioFile::WaveAudioFile(const std::string& filePath) :
    WaveAudioFile(filePath.c_str())
{}

WaveAudioFile::~WaveAudioFile()
{
    close();
}

/*
Open the Wave file and read all the headers.
*/
void WaveAudioFile::open()
{
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
        short pcmFormat = 0;
        m_audioFile.read((char*)&pcmFormat, 2);
        if (pcmFormat != 1 || m_audioFile.fail())
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
            bitsPerSample != 24 && bitsPerSample != 32 &&
            bitsPerSample != 64 || m_audioFile.fail())
            return;
        
        // Read extra bytes.
        if (fmt_size == 18 || fmt_size == 40)
        {
            char extraBytes[24];
            m_audioFile.read(extraBytes, fmt_size-16);
            if (m_audioFile.fail())
                return;
        }

        // Next identifier
        char nextIdentifier[5] = {0, 0, 0, 0, 0};
        m_audioFile.read(nextIdentifier, 4);
        if (m_audioFile.fail())
            return;
        
        // If next identifier is "LIST" identifier.
        if (strcmp(nextIdentifier, "LIST") == 0)
        {
            // "LIST" size.
            int listSize = 0;
            m_audioFile.read((char*)&listSize, 4);
            if (listSize <= 0 || m_audioFile.fail())
                return;
            
            // Read track name.
            char trackName[listSize+1];
            memset(trackName, 0, listSize+1);
            m_audioFile.read(trackName, listSize);
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
        
        // Then the data will be streamed.

        // Store headers data.
        setNumChannels(channels);
        setSampleRate(sampleRate);
        setBytesPerSample(bitsPerSample/8);
        setSizeStream(audioDataSize);
        updateBuffersSize();
        fileOpened();
    }
}

/*
Close the Wave file and release resources.
*/
void WaveAudioFile::close()
{
    if (m_audioFile.is_open())
        m_audioFile.close();
}

/*
Read from the audio file and put it into
the temporary buffer before going into the 
ring buffer.
*/
void WaveAudioFile::readDataFromFile()
{
    if (!m_audioFile.is_open() || streamSizeInBytes() == 0)
        return;

    if (readPos() == streamSizeInBytes())
    {
        endFile();
        return;
    }
    
    size_t readSize = minimumSizeTemporaryBuffer();
    if (readPos() + readSize > streamSizeInBytes())
        readSize = streamSizeInBytes() - readPos();
    
    char data[readSize];
    memset(data, 0, readSize);

    m_audioFile.read(data, readSize);

    if (m_audioFile.fail())
    {
        endFile();
        return;
    }

    insertDataInfoTmpBuffer(data, readSize);

    incrementReadPos(readSize);
}
}