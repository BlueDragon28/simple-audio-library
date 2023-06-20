#ifndef SIMPLEAUDIOLIBRARY_SNDAUDIOFILE_H_
#define SIMPLEAUDIOLIBRARY_SNDAUDIOFILE_H_

#include "Common.h"
#include "AbstractAudioFile.h"
#include <sndfile.hh>
#include <memory>

namespace SAL
{
/*
Open files with the libsndfile audio library.
*/
class SAL_EXPORT_DLL SndAudioFile : public AbstractAudioFile
{
public:
    SndAudioFile(const std::string& filePath);
    virtual ~SndAudioFile();

protected:
    /*
    Get data from the libsndfile library and put it into
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
    Open the file with the libsndfile library.
    */
    void open();

    std::unique_ptr<SndfileHandle> m_file;
};
}

#endif // SIMPLEAUDIOLIBRARY_SNDAUDIOFILE_H_