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

protected:
    /*
    Mark the file has ready to stream.
    */
    void fileOpened(bool value = true);

private:
    std::string m_filePath;
    bool m_isOpen;
};
}

#endif // SIMPLE_AUDIO_LIBRARY_ABSTRACTAUDIOFILE_H_