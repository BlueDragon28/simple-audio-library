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

void Player::play()
{}

void Player::pause()
{}

void Player::stop()
{
    Pa_CloseStream(m_paStream.get());
    m_paStream.reset();
    m_isPlaying = false;
    resetStreamInfo();
}

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

void Player::pushFile()
{
    if (m_queueOpenedFile.size() >= m_maxInStreamQueue)
        return;

    AbstractAudioFile* pAudioFile = 
        detectAndOpenFile(m_queueFilePath.at(0));

    if (!m_queueOpenedFile.empty())
    {
        if (!checkStreamInfo(pAudioFile))
            return;
    }

    std::unique_ptr<AbstractAudioFile> pStream(pAudioFile);
    m_queueOpenedFile.push_back(std::move(pStream));
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

void Player::resetStreamInfo()
{
    Pa_StopStream(m_paStream.get());
    m_paStream.reset();
    m_numChannels = 0;
    m_sampleRate = 0;
    m_bytesPerSample = 0;
    m_sampleType = SampleType::UNKNOWN;
}

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
}