#ifndef SIMPLE_AUDIO_LIBRARY_WAVE_AUDIO_FILE_H_
#define SIMPLE_AUDIO_LIBRARY_WAVE_AUDIO_FILE_H_

#include "AbstractAudioFile.h"
#include <fstream>

namespace SAL
{
/*
Interface to stream a Wave audio file.
*/
class SAL_EXPORT_DLL WaveAudioFile : public AbstractAudioFile
{
    WaveAudioFile(const WaveAudioFile& other) = delete;
public:
    /*
    Opening a file *filePath and prepare it
    for streaming.
    */
    WaveAudioFile(const std::string& filePath);
    virtual ~WaveAudioFile();

protected:
    /*
    Read from the audio file and put it into
    the temporary buffer before going into the 
    ring buffer.
    */
    virtual void readDataFromFile() override;

    /*
    Updating the reading position (in frames) of the audio file
    to the new position pos.
    */
    virtual bool updateReadingPos(size_t pos) override;

private:
    /*
    Open the Wave file and read all the headers.
    */
    void open();

    /*
    Close the Wave file and release resources.
    */
    void close();

    // Audio file stream interface.
    std::ifstream m_audioFile;
};
}

#endif // SIMPLE_AUDIO_LIBRARY_WAVE_AUDIO_FILE_H_