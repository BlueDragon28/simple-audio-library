#include "FlacAudioFile.h"
#include <filesystem>

namespace SAL
{
FlacAudioFile::FlacAudioFile(const char* filePath, bool load) :
    AbstractAudioFile(filePath),
    m_isError(false)
{
    if (load)
        open();
}

FlacAudioFile::FlacAudioFile(const std::string& filePath, bool load) :
    FlacAudioFile(filePath.c_str(), load)
{}

FlacAudioFile::~FlacAudioFile()
{}

/*
Opening the Flac file.
*/
void FlacAudioFile::open()
{
    if (!filePath().empty())
    {
        m_isError = true;
        return;
    }

    // Check if the file is existing.
    if (!std::filesystem::exists(filePath()))
    {
        m_isError = true;
        return;
    }

    // Enable md5 checking.
    set_md5_checking(true);

    // Opening the flac file.
    initFile();

    // check if an error occured while initializing the flac file.
    if (m_isError)
        return;
    
    // Reading the matadata of the flac file.
    FLAC__bool result = process_until_end_of_metadata();
    if (!result || m_isError)
    {
        m_isError = true;
        return;
    }

    // Check if the file data are valid.
    if (sampleType() == SampleType::UNKNOWN ||
        numChannels() <= 0 ||
        sampleRate() == 0 ||
        bytesPerSample() <= 0 ||
        streamSizeInBytes() == 0)
    {
        m_isError = true;
        return;
    }

    fileOpened();
}

/*
This virtual method call the init function of the FLAC++ api.
This is set to be able to use another function on a subclass.
*/
void FlacAudioFile::initFile()
{
    FLAC__StreamDecoderInitStatus status = init(filePath());
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK)
        m_isError = true;
}

/*
Read the headers of the flac file.
This method is called by FLAC::Decoder::File.
*/
void FlacAudioFile::metadata_callback(const FLAC__StreamMetadata* metadata)
{
    // Reading the streaming information of the FLAC file.
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
    {
        setNumChannels(metadata->data.stream_info.channels);
        setSampleRate(metadata->data.stream_info.sample_rate);
        setBytesPerSample(metadata->data.stream_info.bits_per_sample/8);
        setSizeStream(
            metadata->data.stream_info.total_samples*numChannels()*bytesPerSample());
        updateBuffersSize();
        setSampleType(SampleType::INT);
    }
}

/*
Read a block from the flac file.
This method is called by FLAC::Decoder::File.
*/
FLAC__StreamDecoderWriteStatus FlacAudioFile::write_callback(const FLAC__Frame* frame, const FLAC__int32* const buffer[])
{

}

/*
Reporting an error while reading the flac file.
This method is called by FLAC::Decoder::File.
*/
void FlacAudioFile::error_callback(FLAC__StreamDecoderErrorStatus status)
{
    m_isError = true;
    endFile(true);
}

/*
Read from the audio file and put it into
the temporary buffer before going into the 
ring buffer.
*/
void FlacAudioFile::readDataFromFile()
{

}

/*
Updating the reading position (in frames) of the audio file
to the new position pos.
*/
bool FlacAudioFile::updateReadingPos(size_t pos)
{
    m_isError = true;
}
}