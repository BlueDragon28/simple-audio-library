#include "FlacAudioFile.h"
#include <filesystem>
#include <cstring>
#include <vector>

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

void FlacAudioFile::open()
{
    if (filePath().empty())
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

void FlacAudioFile::initFile()
{
    FLAC__StreamDecoderInitStatus status = init(filePath());
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK)
        m_isError = true;
}

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

FLAC__StreamDecoderWriteStatus FlacAudioFile::write_callback(const FLAC__Frame* frame, const FLAC__int32* const buffer[])
{
    // Check if the header information of the block is valid.
    if (frame->header.blocksize == 0 ||
        frame->header.bits_per_sample == 0 ||
        frame->header.channels == 0 ||
        frame->header.sample_rate == 0)
    {
        m_isError = true;
        endFile(true);
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    // Check if the header information of the block is the same has the stream.
    if (frame->header.bits_per_sample != bitsPerSample() ||
        frame->header.sample_rate != sampleRate() ||
        frame->header.channels != numChannels())
    {
        m_isError = true;
        endFile(true);
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    // Check if the buffers are valid.
    for (int i = 0; i < numChannels(); i++)
    {
        if (!buffer[i])
        {
            m_isError = true;
            endFile(true);
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }
    }

    // Temporary buffer
    //char data[frame->header.blocksize * numChannels() * bytesPerSample()];
    std::vector<char> data(frame->header.blocksize * numChannels() * bytesPerSample());
    // Hold the position of the buffer in bytes.
    size_t dataPos = 0;

    // Copy each sample into the data buffer.
    for (size_t i = 0; i < frame->header.blocksize; i++)
    {
        for (int j = 0; j < numChannels(); j++)
        {
            memcpy(data.data()+dataPos, &buffer[j][i], bytesPerSample());
            dataPos += bytesPerSample();
        }
    }

    // Copy the buffer (data) into the temporary buffer of (AbstractAudioFile).
    insertDataInfoTmpBuffer(data.data(), dataPos);
    incrementReadPos(dataPos);

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FlacAudioFile::error_callback(FLAC__StreamDecoderErrorStatus status)
{
    m_isError = true;
    endFile(true);
}

void FlacAudioFile::readDataFromFile()
{
    if (m_isError || streamSizeInBytes() == 0)
        return;

    // Reading a block from the flac file.
    if (!process_single())
    {
        m_isError = true;
        endFile(true);
    }

    if (get_state() == FLAC__STREAM_DECODER_END_OF_STREAM)
        endFile(true);
}

bool FlacAudioFile::updateReadingPos(size_t pos)
{
    if (seek_absolute(pos))
        return true;
    else
        return false;
}
}