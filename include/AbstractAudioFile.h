#ifndef SIMPLE_AUDIO_LIBRARY_ABSTRACTAUDIOFILE_H_
#define SIMPLE_AUDIO_LIBRARY_ABSTRACTAUDIOFILE_H_

#include <cstddef>
#include <string>
#include <atomic>
#include <mutex>
#include "RingBuffer.h"
#include "Common.h"

namespace SAL
{
/*
Prototype class to open a file.
The goal of this class is to have 
all the basic tools common to any
files streaming available. This prevent
to rewrite it in all the file class.
*/
class AbstractAudioFile
{
    AbstractAudioFile(const AbstractAudioFile& other) = delete;
public:
    /*
    Opening a file *filePath and prepare it
    for streaming.
    */
    AbstractAudioFile(const char* filePath);
    AbstractAudioFile(const std::string& filePath);
    virtual ~AbstractAudioFile();

    /*
    Return the file path of the audio file.
    */
    inline const std::string& filePath() const noexcept;

    /*
    Return true if the file is ready to stream.
    */
    inline bool isOpen() const noexcept;

    /*
    Is the stream has reached the end.
    */
    inline bool isEnded() const noexcept;

    /*
    Return true if the file stream has reached end.
    */
    inline bool isEndFile() const noexcept;

    /*
    Read data from the file and put it into
    the temporaty buffer.
    */
    void readFromFile();

    /*
    Flush the temporary data into the ring buffer.
    */
    void flush();

    /*
    Seeking a position (in frames) in the audio stream.
    This will clear all the buffers and start playing
    at the position needed if the position if valid.
    */
    void seek(size_t pos);

    /*
    Seeking a position in seconds in the audio stream.
    */
    inline void seekInSeconds(size_t pos) noexcept;

    /*
    Sample rate of the stream.
    */
    inline size_t sampleRate() const noexcept;

    /*
    Number of channels of the stream.
    */
    inline int numChannels() const noexcept;

    /*
    Number of bytes per sample of the stream.
    */
    inline int bytesPerSample() const noexcept;

    /*
    Number of bytes per frame of the stream.
    */
    inline int bytesPerFrame() const noexcept;

    /*
    Number of bits per sample of the stream.
    */
    inline int bitsPerSample() const noexcept;

    /*
    Size in bytes of the audio data.
    */
    inline size_t streamSizeInBytes() const noexcept;

    /*
    Size in samples of the audio data.
    */
    inline size_t streamSizeInSamples() const noexcept;

    /*
    Size in frames of the audio data.
    */
    inline size_t streamSize() const noexcept;

    /*
    Return the size of the buffer available to read.
    */
    inline size_t bufferingSize() const noexcept;

    /*
    Return true if the ring buffer is half filled.
    */
    inline bool isEnoughBuffering() const noexcept;

    /*
    Extract data from the audio files.
    - data = a pointer to a sound buffer.
    - sizeInFrames = the size of the buffers in frames.
    - return the size of data read in frames.
    */
    size_t read(char* data, size_t sizeInFrames);

    /*
    Return the position of the stream in frames.
    */
    inline size_t streamPos() const noexcept;

    /*
    Return the position of the stream in samples.
    */
    inline size_t streamPosInSamples() const noexcept;

    /*
    Return the position of the stream in bytes.
    */
    inline size_t streamPosInBytes() const noexcept;

    /*
    Sample type if it's an integer or floating point number.
    */
    inline SampleType sampleType() const noexcept;

protected:
    /*
    Mark the file has ready to stream.
    */
    inline void fileOpened(bool value = true) noexcept;

    /*
    Pure virtual method who read from the audio
    file and put the data into the temporary buffer.
    */
    virtual void readDataFromFile() = 0;

    /*
    Insert data into the tmp buffer.
    */
    void insertDataInfoTmpBuffer(char* buffer, size_t size);

    /*
    Update sample rate.
    */
    inline void setSampleRate(size_t sampleRate) noexcept;

    /*
    Update number of channels.
    */
    inline void setNumChannels(int numChannels) noexcept;

    /*
    Update bytes per sample.
    */
    inline void setBytesPerSample(int bytesPerSample) noexcept;

    /*
    Update size of the stream in bytes.
    */
    inline void setSizeStream(size_t sizeStream) noexcept;

    /*
    Reset stream position.
    */
    inline void resetStreamPosition() noexcept;

    /*
    Update the buffers size when the 
    audio file header is readed.
    */
    void updateBuffersSize();

    /*
    Minimum size recommanded for the temporary buffer.
    */
    inline size_t minimumSizeTemporaryBuffer() const noexcept;

    /*
    No more data to read.
    */
    inline void endFile(bool value = true) noexcept;

    /*
    Position of the reading of the audio data
    from the audio file.
    */
    inline size_t readPos() const noexcept;

    /*
    Increment the position of the reading position
    of the audio data and set endFile to true when the data
    has reach the end of the file.
    */
    void incrementReadPos(size_t size);

    /*
    Set sampleType, if it's a integer or a floating point number.
    */
    inline void setSampleType(SampleType type) noexcept;

    /*
    Set the starting point of the data in the audio file.
    */
    inline void setDataStartingPoint(size_t pos) noexcept;

    /*
    Get the starting point of the data in the audio file.
    */
    inline size_t dataStartingPoint() const noexcept;

    /*
    Updating the reading position (in frames) of the audio file
    to the new position pos.
    */
    virtual bool updateReadingPos(size_t pos) = 0;

private:
    void updateStreamSizeInfo();
    void updateStreamPosInfo();

    /*
    Resize the temporary buffer.
    */
    void resizeTmpBuffer(size_t size);

    std::string m_filePath;
    bool m_isOpen;

    /*
    Making readFromFile and Flush thread safe.
    */
    std::mutex m_readFromFileMutex;

    // Preventing reading and seeking at the same time.
    std::mutex m_seekMutex;

    /*
    Temprary buffer where data is writen
    in wait to be put into the ring buffer.
    */
    char* m_tmpBuffer;
    size_t m_tmpTailPos;
    size_t m_tmpWritePos;
    size_t m_tmpSizeDataWritten;
    size_t m_tmpSize;
    size_t m_tmpMinimumSize;

    // Ring buffer
    RingBuffer m_ringBuffer;

    // Audio file info.
    std::atomic<size_t> m_sampleRate;
    std::atomic<int> m_numChannels;
    std::atomic<int> m_bytesPerSample;
    std::atomic<int> m_bytesPerFrame;
    std::atomic<size_t> m_sizeStream;
    std::atomic<size_t> m_sizeStreamInSamples;
    std::atomic<size_t> m_sizeStreamInFrames;
    std::atomic<SampleType> m_sampleType;

    // Stream location.
    std::atomic<size_t> m_streamPos;
    std::atomic<size_t> m_streamPosInSamples;
    std::atomic<size_t> m_streamPosInFrames;

    // Indicate where the data start in the audio file.
    size_t m_startDataPos;

    // Indicate no more data need to be readed.
    bool m_endFile;

    // Is the stream has reached the end.
    std::atomic<bool> m_isEnded;

    // Streaming pos from audio file.
    size_t m_readPos;
};

/*
Return the file path of the audio file.
*/
inline const std::string& AbstractAudioFile::filePath() const noexcept
{
    return m_filePath;
}

/*
Return true if the file is ready to stream.
*/
inline bool AbstractAudioFile::isOpen() const noexcept
{
    return m_isOpen;
}

/*
Is the stream has reached the end.
*/
inline bool AbstractAudioFile::isEnded() const noexcept
{
    return m_isEnded;
}

/*
Is the stream has reached the end.
*/
inline bool AbstractAudioFile::isEndFile() const noexcept
{
    return m_endFile;
}

/*
Sample rate of the stream.
*/
inline size_t AbstractAudioFile::sampleRate() const noexcept
{
    return m_sampleRate;
}

/*
Number of channels of the stream.
*/
inline int AbstractAudioFile::numChannels() const noexcept
{
    return m_numChannels;
}

/*
Number of bytes per sample of the stream.
*/
inline int AbstractAudioFile::bytesPerSample() const noexcept
{
    return m_bytesPerSample;
}

/*
Number of bytes per frame of the stream.
*/
inline int AbstractAudioFile::bytesPerFrame() const noexcept
{
    return m_bytesPerFrame;
}

/*
Number of bits per sample of the stream.
*/
inline int AbstractAudioFile::bitsPerSample() const noexcept
{
    return m_bytesPerSample*8;
}

/*
Size in bytes of the audio data.
*/
inline size_t AbstractAudioFile::streamSizeInBytes() const noexcept
{
    return m_sizeStream;
}

/*
Size in samples of the audio data.
*/
inline size_t AbstractAudioFile::streamSizeInSamples() const noexcept
{
    return m_sizeStreamInSamples;
}

/*
Size in frames of the audio data.
*/
inline size_t AbstractAudioFile::streamSize() const noexcept
{
    return m_sizeStreamInFrames;
}

/*
Return the position of the stream in frames.
*/
inline size_t AbstractAudioFile::streamPos() const noexcept
{
    return m_streamPosInFrames;
}

/*
Return the position of the stream in samples.
*/
inline size_t AbstractAudioFile::streamPosInSamples() const noexcept
{
    return m_streamPosInSamples;
}

/*
Return the position of the stream in bytes.
*/
inline size_t AbstractAudioFile::streamPosInBytes() const noexcept
{
    return m_streamPos;
}

/*
Sample type if it's an integer or floating point number.
*/
inline SampleType AbstractAudioFile::sampleType() const noexcept
{
    return m_sampleType;
}

/*
Mark the file has ready to stream.
*/
inline void AbstractAudioFile::fileOpened(bool value) noexcept
{
    m_isOpen = value;
}

/*
Update sample rate.
*/
inline void AbstractAudioFile::setSampleRate(size_t sampleRate) noexcept
{
    m_sampleRate = sampleRate;
}

/*
Update number of channels
*/
inline void AbstractAudioFile::setNumChannels(int numChannels) noexcept
{
    m_numChannels = numChannels;
    updateStreamSizeInfo();
}

/*
Update bytes per sample.
*/
inline void AbstractAudioFile::setBytesPerSample(int bytesPerSample) noexcept
{
    m_bytesPerSample = bytesPerSample;
    updateStreamSizeInfo();
}

/*
Update size of the stream in bytes.
*/
inline void AbstractAudioFile::setSizeStream(size_t sizeStream) noexcept
{
    m_sizeStream = sizeStream;
    updateStreamSizeInfo();
}

/*
Reset stream position.
*/
inline void AbstractAudioFile::resetStreamPosition() noexcept
{
    m_streamPos = 0;
    updateStreamPosInfo();
}

/*
Minimum size recommanded for the temporary buffer.
*/
inline size_t AbstractAudioFile::minimumSizeTemporaryBuffer() const noexcept
{
    return m_tmpMinimumSize - m_tmpTailPos;
}

/*
No more data to read.
*/
inline void AbstractAudioFile::endFile(bool value) noexcept
{
    m_endFile = value;
}

/*
Position of the reading of the audio data
from the audio file.
*/
inline size_t AbstractAudioFile::readPos() const noexcept
{
    return m_readPos;
}

/*
Set sampleType, if it's a integer or a floating point number.
*/
inline void AbstractAudioFile::setSampleType(SampleType type) noexcept
{
    m_sampleType = type;
}

inline size_t AbstractAudioFile::bufferingSize() const noexcept
{
    return m_ringBuffer.readable();
}

/*
Return true if the ring buffer is half filled.
*/
inline bool AbstractAudioFile::isEnoughBuffering() const noexcept
{
    return m_ringBuffer.readable() >= (m_ringBuffer.size() / 2l);
}

/*
Set the starting point of the data in the audio file.
*/
inline void AbstractAudioFile::setDataStartingPoint(size_t pos) noexcept
{
    if (pos < m_sizeStream)
        m_startDataPos = pos;
    else
        m_startDataPos = 0;
}

/*
Get the starting point of the data in the audio file.
*/
inline size_t AbstractAudioFile::dataStartingPoint() const noexcept
{
    return m_startDataPos;
}

/*
Seeking a position in seconds in the audio stream.
*/
inline void AbstractAudioFile::seekInSeconds(size_t pos) noexcept
{
    seek(pos * sampleRate());
}
}

#endif // SIMPLE_AUDIO_LIBRARY_ABSTRACTAUDIOFILE_H_