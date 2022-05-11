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
    const std::string& filePath() const;
    /*
    Return true if the file is ready to stream.
    */
    bool isOpen() const;

    /*
    Is the stream has reached the end.
    */
    bool isEnded() const;

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
    size_t sampleRate() const;

    /*
    Number of channels of the stream.
    */
    int numChannels() const;

    /*
    Number of bytes per sample of the stream.
    */
    int bytesPerSample() const;

    /*
    Number of bits per sample of the stream.
    */
    int bitsPerSample() const;

    /*
    Size in bytes of the audio data.
    */
    size_t streamSizeInBytes() const;

    /*
    Size in samples of the audio data.
    */
    size_t streamSizeInSamples() const;

    /*
    Size in frames of the audio data.
    */
    size_t streamSize() const;

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
    size_t streamPos() const;

    /*
    Return the position of the stream in samples.
    */
    size_t streamPosInSamples() const;

    /*
    Return the position of the stream in bytes.
    */
    size_t streamPosInBytes() const;

    /*
    Sample type if it's an integer or floating point number.
    */
    SampleType sampleType() const;

protected:
    /*
    Mark the file has ready to stream.
    */
    void fileOpened(bool value = true);

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
    void setSampleRate(size_t sampleRate);

    /*
    Update number of channels.
    */
    void setNumChannels(int numChannels);

    /*
    Update bytes per sample.
    */
    void setBytesPerSample(int bytesPerSample);

    /*
    Update size of the stream in bytes.
    */
    void setSizeStream(size_t sizeStream);

    /*
    Reset stream position.
    */
    void resetStreamPosition();

    /*
    Update the buffers size when the 
    audio file header is readed.
    */
    void updateBuffersSize();

    /*
    Minimum size recommanded for the temporary buffer.
    */
    size_t minimumSizeTemporaryBuffer() const;

    /*
    No more data to read.
    */
    void endFile(bool value = true);

    /*
    Position of the reading of the audio data
    from the audio file.
    */
    size_t readPos() const;

    /*
    Increment the position of the reading position
    of the audio data and set endFile to true when the data
    has reach the end of the file.
    */
    void incrementReadPos(size_t size);

    /*
    Set sampleType, if it's a integer or a floating point number.
    */
    void setSampleType(SampleType type);

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