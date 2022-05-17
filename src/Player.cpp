#include "Player.h"
#include <filesystem>
#include <fstream>
#include <cstring>
#include <functional>

#include "WaveAudioFile.h"
#include "CallbackInterface.h"

namespace SAL
{
Player::Player() :
    // PortAudio stream interface.
    m_paStream(nullptr, Pa_CloseStream),

    // If the stream is playing or not.
    m_isPlaying(false),

    // Maximum of same stream in the m_queueOpenedFile queue.
    m_maxInStreamQueue(2),

    // Indicate if the stream is paused and not stopped.
    m_isPaused(false),

    m_isBuffering(false),

    m_callbackInterface(nullptr)
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
    if (!isExisting)
        return;
    
    {
        std::scoped_lock lock(m_queueFilePathMutex);
        m_queueFilePath.push_back(filePath);
    }

    pushFile();

    if (clearQueue && isCurrentPlaying)
        play();
}

/*
Start paying if there is any stream to play.
*/
void Player::play()
{
    if (m_isPlaying)
        return;
    
    std::scoped_lock lock(m_paStreamMutex);
    
    if (m_paStream)
    {
        PaError err = Pa_StartStream(m_paStream.get());
        if (err == paNoError)
            m_isPlaying = true;
        m_isPaused = false;
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
            {
                // Notify that the stream is starting.
                for (const std::unique_ptr<AbstractAudioFile>& file : m_queueOpenedFile)
                {
                    if (!file->isEnded())
                    {
                        startStreamingFile(file->filePath());
                        break;
                    }
                }
                m_isPlaying = true;
            }
            else
            {
                m_isPlaying = false;
            }
        }
    }
}

/*
Pause the stream.
*/
void Player::pause()
{
    std::scoped_lock lock(m_paStreamMutex);
    m_isPaused = true;
    if (m_paStream)
        Pa_StopStream(m_paStream.get());
    m_isPlaying = false;
}

/*
Stop playing and delete the queues.
*/
void Player::stop()
{
    std::scoped_lock lock(m_queueFilePathMutex);
    // Notify that the stream is stopping.
    bool hasAStreamPlaying = false;
    for (std::unique_ptr<AbstractAudioFile>& file : m_queueOpenedFile)
    {
        if (!file->isEnded())
        {
            endStreamingFile(file->filePath());
            hasAStreamPlaying = true;
            break;
        }
    }
    if (!hasAStreamPlaying)
    {
        if (!m_queueOpenedFile.empty())
            endStreamingFile(m_queueOpenedFile.at(0)->filePath());
    }
    m_queueFilePath.clear();
    m_isPlaying = false;
    resetStreamInfo();
}

/*
Return true if the stream if playing.
*/
bool Player::isPlaying() const
{
    std::scoped_lock lock(m_queueOpenedFileMutex);
    if (m_isPlaying && !m_queueOpenedFile.empty())
    {
        if (m_queueOpenedFile.at(m_queueOpenedFile.size()-1)->isEnded())
            return false;
        else
            return true;
    }
    else
        return false;
}

/*
Return true if any of the opened file
are ready.
*/
bool Player::isFileReady() const
{
    for (const std::unique_ptr<AbstractAudioFile>& file : m_queueOpenedFile)
    {
        if (file->isOpen() && !file->isEnded())
            return true;
    }
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
    std::scoped_lock lock(m_queueFilePathMutex, m_queueOpenedFileMutex);
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
    m_queueFilePath.erase(m_queueFilePath.begin());
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
        if (strcmp(headIdentifier, "RIFF") == 0)
        {
            file.seekg(8);
            char waveIdentifier[5];
            memset(waveIdentifier, 0, sizeof(waveIdentifier));
            file.read(waveIdentifier, 4);
            if (strcmp(waveIdentifier, "WAVE") == 0)
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
    std::scoped_lock lock(m_queueOpenedFileMutex, m_paStreamMutex);
    Pa_StopStream(m_paStream.get());
    m_paStream.reset();
    m_queueOpenedFile.clear();
    m_numChannels = 0;
    m_sampleRate = 0;
    m_bytesPerSample = 0;
    m_sampleType = SampleType::UNKNOWN;
    m_isPaused = false;
    m_isBuffering = false;
    if (m_isPlaying)
    {
        if (m_queueFilePath.empty() && m_queueOpenedFile.empty())
            m_isPlaying = false;
    }
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
    std::scoped_lock openedFileMutex(
        m_queueOpenedFileMutex);
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
    else if (m_sampleType == SampleType::UINT)
    {
        if (m_bytesPerSample == 1)
            outParams.sampleFormat = paUInt8;
        else
        {
            resetStreamInfo();
            return false;
        }
    }
    else if (m_sampleType == SampleType::FLOAT)
    {
        if (m_bytesPerSample == 4)
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
        staticPortAudioStreamCallback,
        this);
    
    if (err != paNoError)
    {
        resetStreamInfo();
        return false;
    }
    
    Pa_SetStreamFinishedCallback(pStream, Player::staticPortAudioEndStream);
    
    m_paStream = std::unique_ptr<PaStream, decltype(&Pa_CloseStream)>
        (pStream, Pa_CloseStream);
    
    return true;
}

/*
Static C callback use to make a bridge between
PortAudio and this class.
*/
int Player::staticPortAudioStreamCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags flags,
    void* data)
{
    Player* pPlayer = static_cast<Player*>(data);
    return std::invoke(&Player::streamCallback, pPlayer,
        inputBuffer, outputBuffer, framesPerBuffer);
}

void Player::staticPortAudioEndStream(void* data)
{
    Player* pPlayer = static_cast<Player*>(data);
    std::invoke(&Player::streamEndCallback, pPlayer);
}

/*
Stream callback used to collect audio stream
from the audio file interface and sending it to
PortAudio.
*/
int Player::streamCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long framesPerBuffer)
{
    std::scoped_lock lock(m_queueFilePathMutex, m_queueOpenedFileMutex);
    if (m_queueOpenedFile.empty())
        return paComplete;

    size_t framesWrited = 0;
    bool isBuffering = false;

    if (!m_isBuffering)
    {
        for (std::unique_ptr<AbstractAudioFile>& audioFile : m_queueOpenedFile)
        {
            while (framesWrited < framesPerBuffer && !audioFile->isEnded())
            {
                framesWrited += audioFile->read(static_cast<char*>(outputBuffer)+framesWrited*m_bytesPerSample,
                    framesPerBuffer-framesWrited);
                
                if (audioFile->bufferingSize() == 0)
                    break;
            }

            if (audioFile->bufferingSize() == 0 && !audioFile->isEnded())
            {
                isBuffering = true;
                break;
            }

            if (framesWrited == framesPerBuffer)
                break;
        }
    }

    if (framesWrited < framesPerBuffer)
    {
        memset(
            static_cast<char*>(outputBuffer)+(framesWrited*m_bytesPerSample*m_numChannels),
            0,
            (framesPerBuffer-framesWrited)*m_bytesPerSample*m_numChannels);

        if (isBuffering) 
        {
            m_isBuffering = true; 
            return paContinue;
        }

        if (!m_isBuffering)
            return paComplete;
    }

    return paContinue;
}

/*
    When the stream reach end, this member function
is called.
*/
void Player::streamEndCallback()
{
    if (!m_isPaused && !m_isBuffering)
    {
        {
            // Call end stream callback.
            std::scoped_lock lock(m_queueOpenedFileMutex);
            if (!m_queueOpenedFile.empty())
                endStreamingFile(m_queueOpenedFile.at(0)->filePath());
        }
        resetStreamInfo();
    }
}

/*
Read audio data from file and push it
into the ring buffer and push file from
m_queueFilePath to m_queueOpenedFile.
*/
void Player::update()
{
    pauseIfBuffering();
    updateStreamBuffer();
    continuePlayingIfEnoughBuffering();
    clearUnneededStream();
    pushFile();
    recreateStream();
    checkIfNoStream();
}

bool Player::isPaused() const
{
    return m_isPaused;
}

/*
Pause the stream if buffering.
*/
void Player::pauseIfBuffering()
{
    if (m_isBuffering && !m_isPaused)
    {
        bool isError = false;
        {
            std::scoped_lock lock(m_paStreamMutex);
            m_isPaused = true;
            PaError err = Pa_StopStream(m_paStream.get());
            if (err != paNoError)
                isError = true;
        }
        if (isError)
            resetStreamInfo();
    }
}

/*
Restart stream if buffering enough.
*/
void Player::continuePlayingIfEnoughBuffering()
{
    if (isPlaying() && m_isBuffering)
    {
        bool isStartStreamFailed = false;
        {
            std::scoped_lock lock(m_queueOpenedFileMutex, m_paStreamMutex);
            for (const std::unique_ptr<AbstractAudioFile>& file : m_queueOpenedFile)
            {
                if (!file->isEnded())
                {
                    if (file->isEnoughBuffering())
                    {
                        m_isBuffering = false;
                        m_isPaused = false;
                        PaError err = Pa_StartStream(m_paStream.get());
                        if (err != paNoError)
                            isStartStreamFailed = true;
                    }
                }
            }
        }
        if (isStartStreamFailed)    
            resetStreamInfo();
    }
}

/*
Remove the ended stream of m_queueOpenedFile.
*/
void Player::clearUnneededStream()
{
    std::scoped_lock lock(m_queueOpenedFileMutex);
    
    while (m_queueOpenedFile.size() > 0)
    {
        if (!m_queueOpenedFile.at(0)->isEnded())
            break;
        else
        {
            // Notify that the file ended.
            endStreamingFile(m_queueOpenedFile.at(0)->filePath());
            
            m_queueOpenedFile.erase(m_queueOpenedFile.cbegin());

            // Notify of the new file streaming.
            if (!m_queueOpenedFile.empty())
                startStreamingFile(m_queueOpenedFile.at(0)->filePath());
        }
    }
}

/*
Recreate the PortAudio stream for new files
with different stream info.
*/
void Player::recreateStream()
{
    if (!m_isPlaying)
        return;
    
    if (!m_paStream)
    {
        if (!m_queueOpenedFile.empty())
        {
            if (!createStream())
            {
                resetStreamInfo();
                m_isPlaying = false;
            }
            
            std::scoped_lock lock(m_paStreamMutex);
            PaError err = Pa_StartStream(m_paStream.get());
            if (err == paNoError)
                m_isPlaying = true;
            else
                m_isPaused = false;
        }
    }
}

/*
Update audio stream buffer.
Read from the audio files and push the data
into the ring buffer.
*/
void Player::updateStreamBuffer()
{
    std::scoped_lock lock(m_queueOpenedFileMutex);
    for (std::unique_ptr<AbstractAudioFile>& audioFile : m_queueOpenedFile)
    {
        audioFile->readFromFile();
        audioFile->flush();
    }
}

/*
Update audio stream buffer.
Read from the audio files and push the data
into the ring buffer.
*/
void Player::checkIfNoStream()
{
    std::scoped_lock lock(m_queueFilePathMutex, m_queueOpenedFileMutex);
    if (m_isPlaying && m_queueFilePath.empty() && m_queueOpenedFile.empty())
    {
        m_isPlaying = false;
        m_isPaused = false;
    }
}

/*
Return stream size in frames.
*/
size_t Player::streamSizeInFrames() const
{
    std::scoped_lock lock(m_queueOpenedFileMutex);
    if (!m_queueOpenedFile.empty())
        return m_queueOpenedFile.at(0)->streamSize();
    else
        return 0;
}

/*
Return stream pos in frames.
*/
size_t Player::streamPosInFrames() const
{
    std::scoped_lock lock(m_queueOpenedFileMutex);
    if (!m_queueOpenedFile.empty())
        return m_queueOpenedFile.at(0)->streamPos();
    else
        return 0;
}

/*
Return stream size in seconds.
*/
size_t Player::streamSize() const
{
    std::scoped_lock lock(m_queueOpenedFileMutex);
    if (!m_queueOpenedFile.empty())
    {
        const std::unique_ptr<AbstractAudioFile>& audioFile =
            m_queueOpenedFile.at(0);
        return audioFile->streamSize() / audioFile->sampleRate();
    }
    else 
        return 0;
}

/*
Return stream pos in seconds.
*/
size_t Player::streamPos() const
{
    std::scoped_lock lock(m_queueOpenedFileMutex);
    if (!m_queueOpenedFile.empty())
    {
        const std::unique_ptr<AbstractAudioFile>& audioFile =
            m_queueOpenedFile.at(0);
        return audioFile->streamPos() / audioFile->sampleRate();
    }
    else
        return 0;
}

/*
Call the start file callback.
*/
inline void Player::startStreamingFile(const std::string& filePath)
{
    if (m_callbackInterface)
        std::invoke(&CallbackInterface::callStartFileCallback, 
                    m_callbackInterface,
                    filePath);
}

inline void Player::endStreamingFile(const std::string& filePath)
{
    if (m_callbackInterface)
        std::invoke(&CallbackInterface::callEndFileCallback,
                    m_callbackInterface,
                    filePath);
}
}
