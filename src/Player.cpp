#include "Player.h"
#include "DebugLog.h"
#include "config.h"
#include <filesystem>
#include <fstream>
#include <cstring>
#include <functional>
#include <limits>

#ifdef USE_WAVE
#include "WaveAudioFile.h"
#endif
#ifdef USE_FLAC
#include "FlacAudioFile.h"
#endif
#ifdef USE_LIBSNDFILE
#include "SndAudioFile.h"
#endif
#include "CallbackInterface.h"

// Redefine CLASS_NAME to have the name of the class.
#undef CLASS_NAME
#define CLASS_NAME "AudioPlayer"

namespace SAL
{
Player::Player() :
    // PortAudio stream interface.
    m_paStream(nullptr, Pa_CloseStream),
    m_isClosingStreamTheStream(false),

    // If the stream is playing or not.
    m_isPlaying(false),

    // Maximum of same stream in the m_queueOpenedFile queue.
    m_maxInStreamQueue(2),

    // Indicate if the stream is paused and not stopped.
    m_isPaused(false),

    m_isBuffering(false),

    m_callbackInterface(nullptr),

    m_streamPosLastCallback(std::numeric_limits<size_t>::max()),

    m_doNotCheckFile(false),
    m_isStopping(false)
{}

Player::~Player()
{}

void Player::open(const std::string& filePath, bool clearQueue)
{
#ifndef NDEBUG
    SAL_DEBUG ("Opening file: " + filePath)
#endif

    if (filePath.empty())
    {
        SAL_DEBUG("Opening file failed: file path empty")
        return;
    }
    
    bool isCurrentPlaying = isPlaying();
    if (clearQueue)
    {
        SAL_DEBUG("Opening file: clearing the playing/pending queue")

        stop();
    }

    bool isExisting = std::filesystem::exists(filePath);
    if (!isExisting)
    {
        SAL_DEBUG("Opening file failed: the file do not exists")

        return;
    }
    
    {
        std::scoped_lock lock(m_queueFilePathMutex, m_queueOpenedFileMutex);
        m_queueFilePath.push_back(filePath);
        pushFile();
    }

    if (clearQueue && isCurrentPlaying)
        play();

    SAL_DEBUG("Opening file done")
}

int Player::isReadable(const std::string& filePath) const
{
#ifndef NDEBUG
    SAL_DEBUG("Checking is file " + filePath + " is readable")
#endif

    return checkFileFormat(filePath);
}

void Player::play()
{
    if (m_isPlaying)
        return;

    SAL_DEBUG("Start playing stream")
    
    std::scoped_lock lock(m_paStreamMutex);
    
    if (m_paStream)
    {
        PaError err = Pa_StartStream(m_paStream.get());
        if (err == paNoError)
        {
            if (!m_isPlaying || m_isPaused)
                streamPlayingCallback();
            m_isPlaying = true;
        }
        m_isPaused = false;
    }
    else
    {
        if (!m_queueFilePath.empty())
        {
            std::scoped_lock lock(m_queueFilePathMutex, m_queueOpenedFileMutex);
            pushFile();
        }
        
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
                std::scoped_lock lock(m_queueOpenedFileMutex);
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
                streamPlayingCallback();
            }
            else
            {
                m_isPlaying = false;
            }
        }
    }

    SAL_DEBUG("Start playing stream done")
}

void Player::pause()
{
    SAL_DEBUG("Pausing stream")

    std::scoped_lock lock(m_paStreamMutex);
    m_isPaused = true;
    if (m_paStream)
        Pa_StopStream(m_paStream.get());
    if (m_isPlaying == true)
        streamPausedCallback();
    m_isPlaying = false;

    SAL_DEBUG("Pausing stream done")
}

void Player::stop()
{
    SAL_DEBUG("Stopping stream")

    // Notify that the stream is stopping.
    bool hasAStreamPlaying = false;
    {
        std::scoped_lock lock(m_queueFilePathMutex);
        for (std::unique_ptr<AbstractAudioFile>& file : m_queueOpenedFile)
        {
            if (!file->isEnded())
            {
                endStreamingFile(file->filePath());
                hasAStreamPlaying = true;
                break;
            }
        }
    }
    if (!hasAStreamPlaying)
    {
        std::scoped_lock lock(m_queueOpenedFileMutex);
        if (!m_queueOpenedFile.empty())
            endStreamingFile(m_queueOpenedFile.at(0)->filePath());
    }
    m_queueFilePath.clear();
    m_isPlaying = false;
    streamStoppingCallback();

    // Stopping the stream.
    std::scoped_lock lock(m_paStreamMutex);
    // Prevent streamEndCallback to call endStreamingFile callback.
    m_isStopping = true;
    Pa_StopStream(m_paStream.get());
    resetStreamInfo();
    m_isStopping = false;

    SAL_DEBUG("Stopping stream done")
}

void Player::next()
{
    SAL_DEBUG("Playing next file in the queue")

    if (m_queueOpenedFile.size() > 1 || (m_queueOpenedFile.size() == 1 && m_queueFilePath.size() >= 1))
    {
        // Removing the current stream.
        {
            std::scoped_lock lock(m_queueFilePathMutex, m_queueOpenedFileMutex);
            endStreamingFile(m_queueOpenedFile.at(0)->filePath());
            m_queueOpenedFile.erase(m_queueOpenedFile.cbegin());
            m_doNotCheckFile = false;
        }

        // Close the stream if there is no other stream to play or that don't have the same stream info.
        if (m_queueOpenedFile.empty())
        {
            std::scoped_lock lock(m_paStreamMutex);
            Pa_CloseStream(m_paStream.get());
        }

        // Creating new stream if opened file array is empty.
        while (m_queueOpenedFile.empty() && !m_queueFilePath.empty())
        {
            pushFile();
        }

        recreateStream();

        if (!m_queueOpenedFile.empty())
            startStreamingFile(m_queueOpenedFile.at(0)->filePath());
    }

    SAL_DEBUG("Playing next file in the queue done")
}

bool Player::isPlaying() const
{
    std::scoped_lock lock(m_queueOpenedFileMutex);
    return _isPlaying();
}

bool Player::_isPlaying() const
{
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

bool Player::isFileReady() const
{
    SAL_DEBUG("Checking if there are file ready to play")

    for (const std::unique_ptr<AbstractAudioFile>& file : m_queueOpenedFile)
    {
        if (file->isOpen() && !file->isEnded())
            return true;
    }
    return false;
}

void Player::pushFile()
{
    if (m_queueOpenedFile.size() >= m_maxInStreamQueue ||
        m_queueFilePath.size() == 0 || m_doNotCheckFile)
        return;

    SAL_DEBUG("Starting to stream a new file")

    std::unique_ptr<AbstractAudioFile> pAudioFile(
        detectAndOpenFile(m_queueFilePath.at(0)));
    
    if (!pAudioFile)
    {
        m_queueFilePath.erase(m_queueFilePath.begin());

        SAL_DEBUG("Starting to stream a new file failed: invalid file")

        return;
    }

    if (!m_queueOpenedFile.empty())
    {
        if (!checkStreamInfo(pAudioFile.get()))
        {
            SAL_DEBUG("Starting to stream a new file failed: data information not the same has current stream")

            m_doNotCheckFile = true;
            return;
        }
    }

    m_queueOpenedFile.push_back(std::move(pAudioFile));
    m_queueFilePath.erase(m_queueFilePath.begin());

    SAL_DEBUG("Starting to stream a new file done")
}

AbstractAudioFile* Player::detectAndOpenFile(const std::string& filePath) const
{
    SAL_DEBUG("Detecting audio format type of a file and opening it")

    AbstractAudioFile* pAudioFile = nullptr;

    // Get the file format.
    int format = checkFileFormat(filePath);

    // Create the audio file stream base on the file format (if supported).
    switch (format)
    {
#ifdef USE_WAVE
    case WAVE:
    {
        pAudioFile = new WaveAudioFile(filePath);
    } break;
#endif

#ifdef USE_FLAC
    case FLAC:
    {
        pAudioFile = new FlacAudioFile(filePath);
    } break;
#endif

#ifdef USE_LIBSNDFILE
    case SNDFILE:
    {
        pAudioFile = new SndAudioFile(filePath);
    } break;
#endif

    case UNKNOWN_FILE:
    default:
    {} break;
    }

    SAL_DEBUG("Detecting audio format type of a file and opening it done")
    
    return pAudioFile;
}

int Player::checkFileFormat(const std::string& filePath) const
{
    // Trying to read the file with the different implementation to check which one to use.
#ifdef USE_WAVE
    if (WaveAudioFile(filePath).isOpen())
        return WAVE;
#endif
#ifdef USE_FLAC
    if (FlacAudioFile(filePath).isOpen())
        return FLAC;
#endif
#ifdef USE_LIBSNDFILE
    if (SndAudioFile(filePath).isOpen())
        return SNDFILE;
#endif

    return UNKNOWN_FILE;
}

void Player::_resetStreamInfo()
{
    SAL_DEBUG("Resetting stream informations and closing stream")

    m_paStream.reset();
    m_isClosingStreamTheStream = false;
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
        {
            m_isPlaying = false;
            streamStoppingCallback();
        }
    }
    // Restart checking file.
    m_doNotCheckFile = false;

    SAL_DEBUG("Resetting stream informations and closing stream done")
}

void Player::resetStreamInfo()
{
    std::scoped_lock lock(m_queueOpenedFileMutex);
    _resetStreamInfo();
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
    if (file->streamBytesPerSample() != m_bytesPerSample)
        isSame = false;
    if (file->streamSampleType() != m_sampleType)
        isSame = false;
    return isSame;
}

bool Player::createStream()
{
    SAL_DEBUG("Creating a new stream sink")

    if (m_paStream)
    {
        SAL_DEBUG("Closing current stream sink")

        m_paStream.reset();
    }
    
    if (m_queueOpenedFile.empty())
    {
        SAL_DEBUG("Creating a new stream sink failed: no files to stream")

        return false;
    }

    // Retrieve PCM info from the stream.
    AbstractAudioFile* audioFile;
    audioFile = m_queueOpenedFile.at(0).get();
    m_numChannels = audioFile->numChannels();
    m_sampleRate = audioFile->sampleRate();
    m_bytesPerSample = audioFile->streamBytesPerSample();
    m_sampleType = audioFile->streamSampleType();

    // Check if the PCM info are valid.
    if (m_numChannels == 0 || m_sampleRate == 0 ||
        m_bytesPerSample == 0 || m_sampleType == SampleType::UNKNOWN)
    {
        SAL_DEBUG("Creating a new stream sink failed: audio data informations not valid")

        _resetStreamInfo();
        return false;
    }

    // Retrieve the default output device.
    int defaultOutputDevice = Pa_GetDefaultOutputDevice();

    // Set the info of the PortAudio stream.
    PaStreamParameters outParams = {};
    outParams.device = defaultOutputDevice;
    outParams.channelCount = m_numChannels;
    if (m_sampleType == SampleType::FLOAT)
    {
        if (m_bytesPerSample == 4)
            outParams.sampleFormat = paFloat32;
        else
        {
            SAL_DEBUG("Creating a new stream sink failed: not valid floating point number")

            resetStreamInfo();
            return false;
        }
    }
    outParams.suggestedLatency = Pa_GetDeviceInfo(defaultOutputDevice)->defaultHighOutputLatency;
    outParams.hostApiSpecificStreamInfo = nullptr;

    // Create the PortAudio stream.
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
        SAL_DEBUG("Creating a new stream sink failed: creating portaudio stream failed")

        resetStreamInfo();
        return false;
    }
    
    Pa_SetStreamFinishedCallback(pStream, Player::staticPortAudioEndStream);
    
    m_paStream = std::unique_ptr<PaStream, decltype(&Pa_CloseStream)>
        (pStream, Pa_CloseStream);

    // checkStreamInfo may be called before createStream, which lead to a fail even if the next stream is compatible.
    m_doNotCheckFile = false;

    SAL_DEBUG("Creating a new stream sink done")
    
    return true;
}

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

int Player::streamCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long framesPerBuffer)
{
    SAL_DEBUG("Send audio from ring buffer to PortAudio")

    std::scoped_lock lock(m_queueFilePathMutex, m_queueOpenedFileMutex);
    if (m_queueOpenedFile.empty())
    {
        SAL_DEBUG("No audio data to stream, closing the stream")

        return paComplete;
    }

    size_t framesWrited = 0;
    bool isBuffering = false;

    if (!m_isBuffering)
    {
        // Process all the opened files until outputBuffer is full.
        for (std::unique_ptr<AbstractAudioFile>& audioFile : m_queueOpenedFile)
        {
            // Get data from file until the outputBuffer is full and audioFile is not at the end.
            while (framesWrited < framesPerBuffer && !audioFile->isEnded())
            {
                framesWrited += audioFile->read(static_cast<char*>(outputBuffer)+framesWrited*audioFile->streamBytesPerFrame(),
                    framesPerBuffer-framesWrited);
                
                if (audioFile->bufferingSize() == 0)
                    break;
            }

            // If not enough data, pause the stream to let the audio file buffer to fill.
            if (audioFile->bufferingSize() == 0 && (!audioFile->isEnded() && !audioFile->isEndFile()))
            {
                isBuffering = true;
                streamBufferingCallback();
                break;
            }

            // Check if output buffer is full.
            if (framesWrited == framesPerBuffer)
                break;
        }
    }

    // Call stream position change in frames.
    streamPosChangeInFrames(m_queueOpenedFile.at(0)->streamPos());

    if (framesWrited < framesPerBuffer)
    {
        // If the output buffer is not full, fill the end of the buffer with null data (to prevent artefacts).
        memset(
            static_cast<char*>(outputBuffer)+(framesWrited*m_bytesPerSample*m_numChannels),
            0,
            (framesPerBuffer-framesWrited)*m_bytesPerSample*m_numChannels);

        if (isBuffering) 
        {
            SAL_DEBUG("Stream buffering")

            m_isBuffering = true; 
            return paContinue;
        }

        if (!m_isBuffering)
        {
            SAL_DEBUG("No more data to read")

            return paComplete;
        }
    }

    SAL_DEBUG("Send audio from ring buffer to PortAudio done")

    return paContinue;
}

void Player::streamEndCallback()
{
    SAL_DEBUG("End of stream callback")

    if (!m_isPaused && !m_isBuffering)
    {
        {
            // Call end stream callback.
            std::scoped_lock lock(m_queueOpenedFileMutex);
            if (!m_queueOpenedFile.empty() && !m_isStopping)
                endStreamingFile(m_queueOpenedFile.at(0)->filePath());
        }
        //resetStreamInfo();
        m_isClosingStreamTheStream = true;
    }

    SAL_DEBUG("End of stream callback done")
}

void Player::update()
{
    SAL_DEBUG("update loop: reading data from file and clearing unneeded streams")

    static int counter = 0;
    closeStreamWhenNeeded();
    pauseIfBuffering();
    std::scoped_lock lock(m_queueFilePathMutex, m_queueOpenedFileMutex);
    updateStreamBuffer();
    continuePlayingIfEnoughBuffering();
    clearUnneededStream();
    pushFile();
    recreateStream();
    checkIfNoStream();
    streamPosChangeCallback();

    SAL_DEBUG("update loop: reading data from file and clearing unneeded streams done")
}

bool Player::isPaused() const
{
    return m_isPaused;
}

void Player::pauseIfBuffering()
{
    SAL_DEBUG("Buffering: pausing the steam")

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
        {
            SAL_DEBUG("Buffering: pausing the steam failed: cannot pause the stream")

            _resetStreamInfo();
        }
    }

    SAL_DEBUG("Buffering: pausing the steam done")
}

void Player::continuePlayingIfEnoughBuffering()
{
    SAL_DEBUG("Enough buffering, resume stream")

    if (_isPlaying() && m_isBuffering)
    {
        bool isStartStreamFailed = false;
        {
            std::scoped_lock lock(m_paStreamMutex);
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
                        streamEnoughBufferingCallback();
                    }
                }
            }
        }
        if (isStartStreamFailed)    
        {
            SAL_DEBUG("Enough buffering, resume stream failed: starting stream failed")

            _resetStreamInfo();
        }
    }

    SAL_DEBUG("Enough buffering, resume stream done")
}

void Player::clearUnneededStream()
{
    SAL_DEBUG("Clearing unneeded streams")

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
            
            // Restart checking file.
            m_doNotCheckFile = false;
        }
    }

    SAL_DEBUG("Clearing unneeded streams done")
}

void Player::recreateStream()
{
    SAL_DEBUG("Recreating a new stream sink (the new file have not the same data informations)")

    if (!m_isPlaying)
        return;
    
    if (!m_paStream)
    {
        if (!m_queueOpenedFile.empty())
        {
            if (!createStream())
            {
                _resetStreamInfo();
                m_isPlaying = false;
            }
            
            std::scoped_lock lock(m_paStreamMutex);
            PaError err = Pa_StartStream(m_paStream.get());
            if (err == paNoError)
            {
                SAL_DEBUG("Recreating a new stream sink failed: starting the stream failed")

                m_isPlaying = true;
                
                // Call end stream callback.
                if (!m_queueOpenedFile.empty())
                    startStreamingFile(m_queueOpenedFile.at(0)->filePath());
            }
            else
            {
                m_isPaused = false;
            }
        }
    }

    SAL_DEBUG("Recreating a new stream sink done")
}

void Player::closeStreamWhenNeeded()
{
    if (m_isClosingStreamTheStream)
    {
        std::scoped_lock lock(m_paStreamMutex);
        resetStreamInfo();
    }
}

void Player::updateStreamBuffer()
{
    SAL_DEBUG("Reading data from files")

    for (std::unique_ptr<AbstractAudioFile>& audioFile : m_queueOpenedFile)
    {
        audioFile->readFromFile();
        audioFile->flush();
    }

    SAL_DEBUG("Reading data from files done")
}

void Player::checkIfNoStream()
{
    if (m_isPlaying && m_queueFilePath.empty() && m_queueOpenedFile.empty())
    {
        m_isPlaying = false;
        m_isPaused = false;
    }
}

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

inline void Player::streamPosChangeInFrames(size_t streamPos)
{
    if (m_callbackInterface)
        std::invoke(&CallbackInterface::callStreamPosChangeCallback,
                    m_callbackInterface,
                    streamPos,
                    TimeType::FRAMES);
}

inline void Player::streamPosChangeCallback()
{
    if (m_callbackInterface && !m_queueOpenedFile.empty() && _isPlaying())
    {
        size_t pos = _streamPos(TimeType::SECONDS);
        if (pos != m_streamPosLastCallback)
        {
            m_streamPosLastCallback = pos;
            std::invoke(&CallbackInterface::callStreamPosChangeCallback,
                        m_callbackInterface,
                        m_streamPosLastCallback,
                        TimeType::SECONDS);
        }
    }
}

inline void Player::streamPausedCallback()
{
    if (m_callbackInterface)
        std::invoke(&CallbackInterface::callStreamPausedCallback,
                    m_callbackInterface);
}

inline void Player::streamPlayingCallback()
{
    if (m_callbackInterface)
        std::invoke(&CallbackInterface::callStreamPlayingCallback,
                    m_callbackInterface);
}

inline void Player::streamStoppingCallback()
{
    if (m_callbackInterface)
        std::invoke(&CallbackInterface::callStreamStoppingCallback,
                    m_callbackInterface);
}

inline void Player::streamBufferingCallback()
{
    if (m_callbackInterface)
        std::invoke(&CallbackInterface::callStreamBufferingCallback,
                    m_callbackInterface);
}

inline void Player::streamEnoughBufferingCallback()
{
    if (m_callbackInterface)
        std::invoke(&CallbackInterface::callStreamEnoughBufferingCallback,
                    m_callbackInterface);
}
}
