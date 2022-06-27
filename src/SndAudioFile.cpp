#include "SndAudioFile.h"
#include <cstring>

namespace SAL
{
SndAudioFile::SndAudioFile(const char* filePath) :
    AbstractAudioFile(filePath),
    m_file(new SndfileHandle(filePath))
{
    if (!*m_file.get())
    {
        return;
    }

    open();
}

SndAudioFile::SndAudioFile(const std::string& filePath) :
    SndAudioFile(filePath.c_str())
{}

SndAudioFile::~SndAudioFile()
{}

/*
Open the file with the libsndfile library.
*/
void SndAudioFile::open()
{
    // Opening the file with libsndfile library.
    m_file = std::unique_ptr<SndfileHandle>(new SndfileHandle(filePath()));
    if (!*m_file.get())
        return;
    
    // Get header information of the audio file.
    setNumChannels(m_file->channels());
    setSampleRate(m_file->samplerate());

    if (numChannels() <= 0 || sampleRate() <= 0)
        return;
    
    // Get bytes per sample.
    int format = m_file->format() & SF_FORMAT_SUBMASK;
    switch (format)
    {
    case SF_FORMAT_PCM_S8:
    case SF_FORMAT_PCM_U8:
    {
        setBytesPerSample(1);
    } break;

    case SF_FORMAT_PCM_16:
    case SF_FORMAT_VORBIS:
    {
        setBytesPerSample(2);
    } break;

    case SF_FORMAT_PCM_24:
    {
        setBytesPerSample(3);
    } break;

    case SF_FORMAT_PCM_32:
    case SF_FORMAT_FLOAT:
    {
        setBytesPerSample(4);
    } break;

    default:
    {
        return;
    } break;
    }

    // Set sample type
    switch (format)
    {
    case SF_FORMAT_PCM_U8:
    {
        setSampleType(SampleType::UINT);
    } break;

    case SF_FORMAT_PCM_S8:
    case SF_FORMAT_PCM_16:
    case SF_FORMAT_PCM_24:
    case SF_FORMAT_PCM_32:
    case SF_FORMAT_VORBIS:
    {
        setSampleType(SampleType::INT);
    } break;

    case SF_FORMAT_FLOAT:
    {
        setSampleType(SampleType::FLOAT);
    } break;
    }

    // Set stream size in bytes.
    setSizeStream(m_file->frames() * numChannels() * bytesPerSample());
    if (streamSizeInBytes() == 0)
        return;
    updateBuffersSize();

    // The file opened successfully.
    fileOpened();
}

/*
Get data from the libsndfile library and put it into
the temporary buffer before going into the 
ring buffer.
*/
void SndAudioFile::readDataFromFile()
{
    if (streamSizeInBytes() == 0 && !m_file && !*m_file.get())
        return;
    
    // Get the recommended size of the tmp buffer.
    size_t readSize = minimumSizeTemporaryBuffer();
    if (readPos() + readSize >= streamSizeInBytes())
        readSize = streamSizeInBytes() - readPos();
    
    // Allocate an array of chars to store the audio data.
    char data[readSize];
    memset(data, 0, readSize);

    // Get the number of items in the sample.
    size_t readItems = readSize / bytesPerSample();
    // Number of items read.
    size_t itemsRead = 0;

    // Retrieve the data from the libsndfile library.
    // If the data is of integer format.
    if (sampleType() == SampleType::INT || sampleType() == SampleType::UINT)
    {
        // Create a tmp int buffer and copy the stream inside.
        int tmpData[readItems];
        memset(tmpData, 0, readSize);
        itemsRead = m_file->read(tmpData, readItems);

        // Convert the data from int to the appropriate data type.
        if (bytesPerSample() == 8)
        {
            char* charData = data;
            for (size_t i = 0; i < itemsRead; i++)
                *charData++ = tmpData[i];
        }
        else if (bytesPerSample() == 16)
        {
            short* shortData = reinterpret_cast<short*>(data);
            for (size_t i = 0; i < itemsRead; i++)
                *shortData++ = tmpData[i];
        }
        else if (bytesPerSample() == 24)
        {
            FakeInt24* int24Data = reinterpret_cast<FakeInt24*>(data);
            for (size_t i = 0; i < itemsRead; i++)
                memcpy(int24Data++, &tmpData[i], 3);
        }
        else if (bytesPerSample() == 32)
        {
            memcpy(data, tmpData, itemsRead * bytesPerSample());
        }
    }
    // If the data is of float format.
    if (sampleType() == SampleType::FLOAT && bytesPerSample() == 32)
    {
        // Create a tmp input buffer and copy the stream inside.
        float tmpData[readItems];
        memset(tmpData, 0, readSize);
        itemsRead = m_file->read(tmpData, readItems);
        memcpy(data, tmpData, itemsRead * bytesPerSample());
    }

    // Convert itemsRead to bytes.
    itemsRead *= bytesPerSample();

    // Push the buffer to the tmp buffer.
    if (itemsRead > 0)
    {
        insertDataInfoTmpBuffer(data, itemsRead);
        incrementReadPos(itemsRead);
    }
}

/*
Updating the reading position (in frames) of the audio file
to the new position pos.
*/
bool SndAudioFile::updateReadingPos(size_t pos)
{
    size_t newPosition = m_file->seek(pos, SF_SEEK_SET);
    if (newPosition >= 0)
        return true;
    else
        return false;
}
}