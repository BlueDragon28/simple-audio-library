#include "SndAudioFile.h"
#include "DebugLog.h"
#include <cstring>
#include <vector>

// Define CLASS_NAME to have the name of the class.
const std::string CLASS_NAME = "SndAudioFile";

namespace SAL
{
SndAudioFile::SndAudioFile(const std::string& filePath) :
    AbstractAudioFile(filePath),
    m_file(new SndfileHandle(filePath))
{
    if (!*m_file.get())
    {
        return;
    }

    open();
}

SndAudioFile::~SndAudioFile()
{}

void SndAudioFile::open()
{
#ifndef NDEBUG
    SAL_DEBUG("Opening file " + filePath());
#endif

    // Opening the file with libsndfile library.
    m_file = std::unique_ptr<SndfileHandle>(new SndfileHandle(filePath()));
    if (!*m_file.get())
    {
        SAL_DEBUG("Opening file failed: sndfile cannot read file")

        return;
    }
    
    // Get header information of the audio file.
    setNumChannels(m_file->channels());
    setSampleRate(m_file->samplerate());

    if (numChannels() <= 0 || sampleRate() <= 0)
    {
        SAL_DEBUG("Opening file failed: number of channels and/or sample rate invalid")

        return;
    }
    
    // Check if it's a valid encoding.
    int format = m_file->format() & SF_FORMAT_SUBMASK;
    switch (format)
    {
    case SF_FORMAT_PCM_U8:
    case SF_FORMAT_PCM_S8:
    case SF_FORMAT_PCM_16:
    case SF_FORMAT_PCM_24:
    case SF_FORMAT_PCM_32:
    case SF_FORMAT_FLAC:
    case SF_FORMAT_ALAW:
    case SF_FORMAT_ULAW:
    case SF_FORMAT_IMA_ADPCM:
    case SF_FORMAT_MS_ADPCM:
    case SF_FORMAT_GSM610:
    case SF_FORMAT_VOX_ADPCM:
    case SF_FORMAT_NMS_ADPCM_16:
    case SF_FORMAT_NMS_ADPCM_24:
    case SF_FORMAT_NMS_ADPCM_32:
    case SF_FORMAT_G721_32:
    case SF_FORMAT_G723_24:
    case SF_FORMAT_G723_40:
    case SF_FORMAT_DWVW_12:
    case SF_FORMAT_DWVW_16:
    case SF_FORMAT_DWVW_24:
    case SF_FORMAT_DWVW_N:
    case SF_FORMAT_DPCM_8:
    case SF_FORMAT_DPCM_16:
    case SF_FORMAT_VORBIS:
    case SF_FORMAT_OPUS:
    case SF_FORMAT_ALAC_16:
    case SF_FORMAT_ALAC_20:
    case SF_FORMAT_ALAC_24:
    case SF_FORMAT_ALAC_32:
    case SF_FORMAT_MPEG_LAYER_I:
    case SF_FORMAT_MPEG_LAYER_II:
    case SF_FORMAT_MPEG_LAYER_III:
    {
        /*
        The sndfile library do not allow retrieving audio pcm 
        of 8 and 24 bits. So, every pcm is converted to 32 bits float.
        */
        setBytesPerSample(4);
        setSampleType(SampleType::FLOAT);
    } break;

    // Not a compatible format, leaving.
    default:
    {
        SAL_DEBUG("Opening file failed: incompatible file format")

        return;
    } break;
    }

    // Set stream size in bytes.
    setSizeStream(m_file->frames() * numChannels() * bytesPerSample());
    if (streamSizeInBytes() == 0)
    {
        SAL_DEBUG("Opening file failed: invalid stream size")

        return;
    }
    updateBuffersSize();

    // The file opened successfully.
    fileOpened();

    SAL_DEBUG("Opening file done")
}

void SndAudioFile::readDataFromFile()
{
    SAL_DEBUG("Read data from file")

    if (streamSizeInBytes() == 0 || !m_file || !*m_file.get() || sampleType() != SampleType::FLOAT)
        return;
    
    // Get the recommended size of the tmp buffer.
    size_t readSize = minimumSizeTemporaryBuffer();
    if (readPos() + readSize >= streamSizeInBytes())
        readSize = streamSizeInBytes() - readPos();
    
    // Allocate an array of chars to store the audio data.
    std::vector<char> data(readSize);
    memset(data.data(), 0, readSize);

    // Get the number of items in the sample.
    size_t readItems = readSize / bytesPerSample();

    // Retrieve the data from the libsndfile library and listen
    // to the number of bytes read.
    size_t itemsRead = m_file->read((float*)data.data(), readItems);

    // Convert itemsRead to bytes.
    itemsRead *= bytesPerSample();

    // Push the buffer to the tmp buffer.
    if (itemsRead > 0)
    {
        insertDataInfoTmpBuffer(data.data(), itemsRead);
        incrementReadPos(itemsRead);
    }

    SAL_DEBUG("Read data from file done")
}

bool SndAudioFile::updateReadingPos(size_t pos)
{
    SAL_DEBUG("Update reading position")

    size_t newPosition = m_file->seek(pos, SF_SEEK_SET);
    if (newPosition >= 0)
        return true;
    else
        return false;
}
}