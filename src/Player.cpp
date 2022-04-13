#include "Player.h"
#include <filesystem>
#include <fstream>
#include <cstring>

#include "WaveAudioFile.h"

namespace SAL
{
Player::Player() :
    // PortAudio stream interface.
    m_paStream(nullptr, Pa_CloseStream),

    // If the stream is playing or not.
    m_isPlaying(false),

    // Maximum of same stream in the m_queueOpenedFile queue.
    m_maxInStreamQueue(2)
{}

Player::~Player()
{}

/*
Add a new file into the queue and stream it when the queue is empty.
filePath - path to the file.
clearQueue - clear the queue and stop stream and use this file has
current stream. If the previous file was playing, the new one will
play too.
*/
void Player::open(const char* filePath, bool clearQueue)
{
    open(std::string(filePath), clearQueue);
}

void Player::open(const std::string& filePath, bool clearQueue)
{
    if (filePath.empty())
        return;
    
    bool isCurrentPlaying = isPlaying();
    if (clearQueue)
        stop();

    bool isExisting = std::filesystem::exists(filePath);
    m_queueFilePath.push_back(filePath);

    pushFile();

    if (isCurrentPlaying)
        play();
}

/*
Start paying if there is any stream to play.
*/
void Player::play()
{
    if (m_isPlaying)
        return;
    
    if (m_paStream)
    {
        PaError err = Pa_StartStream(m_paStream.get());
        if (err == paNoError)
            m_isPlaying = true;
    }
    else
    {
        if (!m_queueFilePath.empty())
            pushFile();
        
        if (!m_queueOpenedFile.empty())
        {
            if (!createStream())
            {
                resetStreamInfo();
                m_isPlaying = false;
            }
            PaError err = Pa_StartStream(m_paStream.get());
            if (err == paNoError)
                m_isPlaying = true;
            else
                m_isPlaying = false;
        }
    }
}

/*
Pause the stream.
*/
void Player::pause()
{
    if (m_paStream)
        Pa_StopStream(m_paStream.get());
    m_isPlaying = false;
}

/*
Stop playing and delete the queues.
*/
void Player::stop()
{
    Pa_CloseStream(m_paStream.get());
    m_paStream.reset();
    m_isPlaying = false;
    resetStreamInfo();
}

/*
Return true if the stream if playing.
*/
bool Player::isPlaying() const
{
    if (m_isPlaying && !m_queueOpenedFile.empty())
    {
        if (m_queueOpenedFile.at(0)->isEnded())
            return false;
        else
            return true;
    }
    else
        return false;
}

/*
Remove ended file from m_queueOpenedFile and
add file from m_queueFilePath if m_queueOpenedFile
is empty or if they have the same stream info
then the same file in m_queueOpenedFile.
*/
void Player::pushFile()
{
    if (m_queueOpenedFile.size() >= m_maxInStreamQueue ||
        m_queueFilePath.size() == 0)
        return;

    std::unique_ptr<AbstractAudioFile> pAudioFile(
        detectAndOpenFile(m_queueFilePath.at(0)));
    
    if (!pAudioFile)
    {
        m_queueFilePath.erase(m_queueFilePath.begin());
        return;
    }

    if (!m_queueOpenedFile.empty())
    {
        if (!checkStreamInfo(pAudioFile.get()))
            return;
    }

    m_queueOpenedFile.push_back(std::move(pAudioFile));
}

/*
Check what type is the file and opening it.
*/
AbstractAudioFile* Player::detectAndOpenFile(const std::string& filePath)
{
    AbstractAudioFile* pAudioFile = nullptr;
    std::ifstream file(filePath, std::ifstream::binary);
    if (file.is_open())
    {
        char headIdentifier[5];
        memset(headIdentifier, 0, sizeof(headIdentifier));

        file.read(headIdentifier, 4);

        // Check if it's an audio file.
        if (strcmp(headIdentifier, "RIFF") != 0)
        {
            file.seekg(8);
            char waveIdentifier[5];
            memset(waveIdentifier, 0, sizeof(waveIdentifier));
            file.read(waveIdentifier, 4);
            if (strcmp(waveIdentifier, "WAVE") != 0)
            {
                pAudioFile = new WaveAudioFile(filePath);
                return pAudioFile;
            }
        }
    }
    return pAudioFile;
}

/*
Reset stream info.
*/
void Player::resetStreamInfo()
{
    Pa_StopStream(m_paStream.get());
    m_paStream.reset();
    m_queueOpenedFile.erase(m_queueOpenedFile.cbegin());
    m_numChannels = 0;
    m_sampleRate = 0;
    m_bytesPerSample = 0;
    m_sampleType = SampleType::UNKNOWN;
}

/*
Check if the audio file (file) have the info than the
currently played file.
*/
bool Player::checkStreamInfo(const AbstractAudioFile* const file) const
{
    if (!file)
        return false;

    bool isSame = true;
    if (file->numChannels() != m_numChannels)
        isSame = false;
    if (file->sampleRate() != m_sampleRate)
        isSame = false;
    if (file->bytesPerSample() != m_bytesPerSample)
        isSame = false;
    if (file->sampleType() != m_sampleType)
        isSame = false;
    return isSame;
}

/*
Create the PaStream.
*/
bool Player::createStream()
{
    if (m_paStream)
        m_paStream.reset();
    
    if (m_queueOpenedFile.empty())
        return false;
    
    std::unique_ptr<AbstractAudioFile>& audioFile =
        m_queueOpenedFile.at(0);
    
    m_numChannels = audioFile->numChannels();
    m_sampleRate = audioFile->sampleRate();
    m_bytesPerSample = audioFile->bytesPerSample();
    m_sampleType = audioFile->sampleType();

    if (m_numChannels == 0 || m_sampleRate == 0 ||
        m_bytesPerSample == 0 || m_sampleType == SampleType::UNKNOWN)
    {
        resetStreamInfo();
        return false;
    }

    int defaultOutputDevice = Pa_GetDefaultOutputDevice();

    PaStreamParameters outParams = {};
    outParams.device = defaultOutputDevice;
    outParams.channelCount = m_numChannels;
    if (m_sampleType == SampleType::INT)
    {
        if (m_bytesPerSample == 1)
            outParams.sampleFormat = paInt8;
        else if (m_bytesPerSample == 2)
            outParams.sampleFormat = paInt16;
        else if (m_bytesPerSample == 3)
            outParams.sampleFormat = paInt24;
        else if (m_bytesPerSample == 4)
            outParams.sampleFormat = paInt32;
        else
        {
            resetStreamInfo();
            return false;
        }
    }
    else if (m_sampleType == SampleType::FLOAT)
    {
        if (m_bytesPerSample == 32)
            outParams.sampleFormat = paFloat32;
        else
        {
            resetStreamInfo();
            return false;
        }
    }
    else
    {
        resetStreamInfo();
        return false;
    }
    outParams.suggestedLatency = Pa_GetDeviceInfo(defaultOutputDevice)->defaultHighOutputLatency;
    outParams.hostApiSpecificStreamInfo = nullptr;

    PaStream* pStream;
    PaError err = Pa_OpenStream(
        &pStream,
        nullptr,
        &outParams,
        (double)m_sampleRate,
        6144,
        paNoFlag,
        nullptr,
        this);
    
    if (err != paNoError)
        return false;
    
    m_paStream = std::unique_ptr<PaStream, decltype(&Pa_CloseStream)>
        (pStream, Pa_CloseStream);
    
    return true;
}
}