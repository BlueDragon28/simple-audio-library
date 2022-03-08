#ifndef SIMPLE_AUDIO_LIBRARY_ABSTRACTAUDIOFILE_H_
#define SIMPLE_AUDIO_LIBRARY_ABSTRACTAUDIOFILE_H_

#include <cstddef>
#include <string>

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
    Resize the temporary buffer.
    */
    void resizeTmpBuffer(size_t size);

    /*
    Return a pointer to the ptr buffer.
    */
    char* tmpBufferPtr();

private:
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
};
}

#endif // SIMPLE_AUDIO_LIBRARY_ABSTRACTAUDIOFILE_H_