#ifndef SIMPLEAUDIOLIBRARY_FLACAUDIOFILE_H_
#define SIMPLEAUDIOLIBRARY_FLACAUDIOFILE_H_

#include "AbstractAudioFile.h"
#include <FLAC++/decoder.h>

namespace SAL
{
/*
Interface to stream a FLAC audio file.
*/
class FlacAudioFile : public AbstractAudioFile, protected FLAC::Decoder::File
{
    FlacAudioFile(const FlacAudioFile&) = delete;
public:
    /*
    Opening a file *filePath and prepare it
    for streaming.
    The load paremeter indicate if the file must be load or not.
    */
    FlacAudioFile(const char* filePath);
    FlacAudioFile(const std::string& filePath);
    virtual ~FlacAudioFile();

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

    /*
    Read a block from the flac file.
    This method is called by FLAC::Decoder::File.
    */
    virtual FLAC__StreamDecoderWriteStatus write_callback(const FLAC__Frame* frame, const FLAC__int32* const buffer[]) override;

    /*
    Read the headers of the flac file.
    This method is called by FLAC::Decoder::File.
    */
    virtual void metadata_callback(const FLAC__StreamMetadata* metadata) override;

    /*
    Reporting an error while reading the flac file.
    This method is called by FLAC::Decoder::File.
    */
    virtual void error_callback(FLAC__StreamDecoderErrorStatus status) override;

    /*
    Opening the Flac file.
    */
    void open();

private:
    // Is there an error while reading the file.
    bool m_isError;
};
}

#endif // SIMPLEAUDIOLIBRARY_FLACAUDIOFILE_H_