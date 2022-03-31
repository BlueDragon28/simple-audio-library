#ifndef SIMPLE_AUDIO_LIBRARY_ABSTRACTAUDIOFILE_H_
#define SIMPLE_AUDIO_LIBRARY_ABSTRACTAUDIOFILE_H_

#include <cstddef>
#include <string>
#include <atomic>
#include "RingBuffer.h"

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
    Read data from the file and put it into
    the temporaty buffer.
    */
    void readFromFile();

    /*
    Flush the temporary data into the ring buffer.
    */
    void flush();

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

private:
    void updateStreamSizeInfo();

    /*
    Resize the temporary buffer.
    */
    void resizeTmpBuffer(size_t size);

    std::string m_filePath;
    bool m_isOpen;

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
};
}

#endif // SIMPLE_AUDIO_LIBRARY_ABSTRACTAUDIOFILE_H_