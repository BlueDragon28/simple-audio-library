#include "SndAudioFile.h"

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
Read from the audio file and put it into
the temporary buffer before going into the 
ring buffer.
*/
void SndAudioFile::readDataFromFile()
{

}

/*
Updating the reading position (in frames) of the audio file
to the new position pos.
*/
bool SndAudioFile::updateReadingPos(size_t pos)
{
    return false;
}
}