#include "Player.h"
#include "Common.h"
#include "DebugLog.h"
#include "config.h"
#include <filesystem>
#include <fstream>
#include <cstring>
#include <functional>
#include <limits>
#include <mutex>
#include <portaudio.h>

#ifdef WIN32
#include "UTFConvertion.h"
#endif

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

// Define CLASS_NAME to have the name of the class.
const std::string CLASS_NAME = "Player";

namespace SAL
{
int Player::_destroyStream(void* stream)
{
    return Pa_CloseStream(stream);
}

Player::Player() :
    // PortAudio stream interface.
    m_paStream(nullptr, Pa_CloseStream),

    m_backendAudio(getSystemDefaultBackendAudio()),

    m_isClosingStreamTheStream(false),

    // If the stream is playing or not.
    m_isPlaying(false),

    // Indicate if the stream is paused and not stopped.
    m_isPaused(false),

    m_isBuffering(false),

    // Maximum of same stream in the m_queueOpenedFile queue.
    m_maxInStreamQueue(2),

    m_callbackInterface(nullptr),

    m_streamPosLastCallback(std::numeric_limits<size_t>::max()),

    m_doNotCheckFile(false),
    m_isStopping(false)
{
    retrieveAvailableHostApi();
}

Player::~Player()
{}

void Player::open(const std::string& filePath, bool clearQueue)
{
    SAL_DEBUG_EVENTS("Opening file: " + filePath)

    if (filePath.empty())
    {
        SAL_DEBUG_EVENTS("Opening file failed: file path empty")
        return;
    }
    
    bool isCurrentPlaying = isPlaying();
    if (clearQueue)
    {
        SAL_DEBUG_EVENTS("Opening file: clearing the playing/pending queue")

        stop();
    }

#ifdef WIN32
    bool isExisting = std::filesystem::exists(UTFConvertion::toWString(filePath));
#else
    bool isExisting = std::filesystem::exists(filePath);
#endif
    if (!isExisting)
    {
        SAL_DEBUG_EVENTS("Opening file failed: the file do not exists")

        return;
    }
    
    {
        std::scoped_lock lock(m_queueFilePathMutex, m_queueOpenedFileMutex);
        m_queueFilePath.push_back(filePath);
        pushFile();
    }

    if (clearQueue && isCurrentPlaying)
        play();

    SAL_DEBUG_EVENTS("Opening file done")
}

int Player::isReadable(const std::string& filePath) const
{
    SAL_DEBUG_EVENTS("Checking is file " + filePath + " is readable")

    return checkFileFormat(filePath);
}

void Player::play()
{
    if (m_isPlaying)
        return;

    SAL_DEBUG_EVENTS("Start playing stream")
    
    std::scoped_lock lock(m_paStreamMutex);
    
    if (m_paStream)
    {
        PaError err = Pa_StartStream(m_paStream.get());
        if (err == paNoError)
        {
            if (!m_isPlaying || m_isPaused)
                streamPlayingCallback();
            m_isPlaying = true;

            SAL_DEBUG_EVENTS("Start playing stream done")
        }
#ifndef NDEBUG
        else
        {
            SAL_DEBUG_EVENTS(std::string("Failed to start playing stream: ") + Pa_GetErrorText(err))
        }
#endif

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

                SAL_DEBUG_EVENTS("Failed to start playing stream")
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

                SAL_DEBUG_EVENTS("Start playing stream done")
            }
            else
            {
                m_isPlaying = false;

                SAL_DEBUG_EVENTS(std::string("Failed to start playing stream: ") + Pa_GetErrorText(err))
            }
        }
    }
}

void Player::pause()
{
    SAL_DEBUG_EVENTS("Pausing stream")

    std::scoped_lock lock(m_paStreamMutex);
    m_isPaused = true;
    if (m_paStream)
        Pa_StopStream(m_paStream.get());
    if (m_isPlaying == true)
        streamPausedCallback();
    m_isPlaying = false;

    SAL_DEBUG_EVENTS("Pausing stream done")
}

void Player::stop()
{
    SAL_DEBUG_EVENTS("Stopping stream")

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

    SAL_DEBUG_EVENTS("Stopping stream done")
}

void Player::next()
{
    SAL_DEBUG_EVENTS("Playing next file in the queue")

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

    SAL_DEBUG_EVENTS("Playing next file in the queue done")
}

void Player::removeNotPlayedPlayback()
{
    SAL_DEBUG_EVENTS("Remove all in queue playback but keep the current one")

    std::scoped_lock lock(m_queueFilePathMutex, m_queueOpenedFileMutex);

    m_queueFilePath.clear();

    if (m_queueOpenedFile.size() >= 2) {
        m_queueOpenedFile.erase(
            m_queueOpenedFile.cbegin()+1,
            m_queueOpenedFile.cend());
    }

    SAL_DEBUG_EVENTS("Remove all in queue playback but keep the current one done")
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

    SAL_DEBUG_LOOP_UPDATE("Preparing a file to be streamed")

    std::unique_ptr<AbstractAudioFile> pAudioFile(
        detectAndOpenFile(m_queueFilePath.at(0)));
    
    if (!pAudioFile)
    {
        m_queueFilePath.erase(m_queueFilePath.begin());

        SAL_DEBUG_LOOP_UPDATE("Preparing a file to be streamed failed: invalid file")

        return;
    }

    if (!m_queueOpenedFile.empty())
    {
        if (!checkStreamInfo(pAudioFile.get()))
        {
            SAL_DEBUG_LOOP_UPDATE("Preparing a file to be streamed failed: data information not the same has current stream")

            m_doNotCheckFile = true;
            return;
        }
    }

    m_queueOpenedFile.push_back(std::move(pAudioFile));
    m_queueFilePath.erase(m_queueFilePath.begin());

    if (m_queueOpenedFile.size() == 1)
    {
        updateStreamBuffer();
    }

    SAL_DEBUG_LOOP_UPDATE("Preparing a file to be streamed done")
}

AbstractAudioFile* Player::detectAndOpenFile(const std::string& filePath) const
{
    SAL_DEBUG_LOOP_UPDATE("Detecting audio format type of a file and opening it")

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

    SAL_DEBUG_LOOP_UPDATE("Detecting audio format type of a file and opening it done")
    
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
    SAL_DEBUG_STREAM_STATUS("Resetting stream informations and closing stream")

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

    SAL_DEBUG_STREAM_STATUS("Resetting stream informations and closing stream done")
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
    SAL_DEBUG_STREAM_STATUS("Creating a new stream sink")

    if (m_paStream)
    {
        SAL_DEBUG_STREAM_STATUS("Closing current stream sink")

        m_paStream.reset();
    }
    
    if (m_queueOpenedFile.empty())
    {
        SAL_DEBUG_STREAM_STATUS("Creating a new stream sink failed: no files to stream")

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
        SAL_DEBUG_STREAM_STATUS("Creating a new stream sink failed: audio data informations not valid")

        _resetStreamInfo();
        return false;
    }

    // Retrieve the default output device.
    PaHostApiIndex hostApiIndex = Pa_HostApiTypeIdToHostApiIndex((PaHostApiTypeId)fromBackendEnumToHostAPI(m_backendAudio));
    PaDeviceIndex outputDevice = Pa_GetHostApiInfo(hostApiIndex)->defaultOutputDevice;

    // Set the info of the PortAudio stream.
    PaStreamParameters outParams = {};
    outParams.device = outputDevice;
    outParams.channelCount = m_numChannels;
    if (m_sampleType == SampleType::FLOAT)
    {
        if (m_bytesPerSample == 4)
            outParams.sampleFormat = paFloat32;
        else
        {
            SAL_DEBUG_STREAM_STATUS("Creating a new stream sink failed: not valid floating point number")

            resetStreamInfo();
            return false;
        }
    }
    outParams.suggestedLatency = Pa_GetDeviceInfo(outputDevice)->defaultHighOutputLatency;
    outParams.hostApiSpecificStreamInfo = nullptr;

    // Create the PortAudio stream.
    PaStream* pStream;
    PaError err = Pa_OpenStream(
        &pStream,
        nullptr,
        &outParams,
        (double)m_sampleRate,
        paFramesPerBufferUnspecified,
        paNoFlag,
        staticPortAudioStreamCallback,
        this);
    
    if (err != paNoError)
    {
        SAL_DEBUG_STREAM_STATUS(std::string("Creating a new stream sink failed: creating portaudio stream failed: ") + Pa_GetErrorText(err))

        resetStreamInfo();
        return false;
    }
    
    Pa_SetStreamFinishedCallback(pStream, Player::staticPortAudioEndStream);
    
    m_paStream = std::unique_ptr<PaStream, decltype(&Pa_CloseStream)>
        (pStream, Pa_CloseStream);

    // checkStreamInfo may be called before createStream, which lead to a fail even if the next stream is compatible.
    m_doNotCheckFile = false;

    SAL_DEBUG_STREAM_STATUS("Creating a new stream sink done")
    
    return true;
}

int Player::staticPortAudioStreamCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    unsigned long flags,
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
    SAL_DEBUG_READ_STREAM("Send audio from ring buffer to PortAudio")

    std::scoped_lock lock(m_queueFilePathMutex, m_queueOpenedFileMutex);
    if (m_queueOpenedFile.empty())
    {
        SAL_DEBUG_READ_STREAM("No audio data to stream, closing the stream")

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
            SAL_DEBUG_READ_STREAM("Stream buffering")

            m_isBuffering = true; 
            return paContinue;
        }

        if (!m_isBuffering)
        {
            SAL_DEBUG_READ_STREAM("No more data to read")

            return paComplete;
        }
    }

    SAL_DEBUG_READ_STREAM("Send audio from ring buffer to PortAudio done")

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
    SAL_DEBUG_LOOP_UPDATE("update loop: reading data from file and clearing unneeded streams")

    closeStreamWhenNeeded();
    pauseIfBuffering();
    {
        std::scoped_lock lock(m_queueFilePathMutex, m_queueOpenedFileMutex);
        updateStreamBuffer();
    }
    continuePlayingIfEnoughBuffering();
    {
        std::scoped_lock lock(m_queueFilePathMutex, m_queueOpenedFileMutex);
        clearUnneededStream();
        pushFile();
    }
    recreateStream();
    std::scoped_lock lock(m_queueFilePathMutex, m_queueOpenedFileMutex);
    checkIfNoStream();
    streamPosChangeCallback();

    SAL_DEBUG_LOOP_UPDATE("update loop: reading data from file and clearing unneeded streams done")
}

bool Player::isPaused() const
{
    return m_isPaused;
}

void Player::pauseIfBuffering()
{
    SAL_DEBUG_LOOP_UPDATE("Check if buffering")

    if (m_isBuffering && !m_isPaused)
    {
        SAL_DEBUG_STREAM_STATUS("Buffering: pausing the stream")

//        bool isError = false;
        {
//            std::scoped_lock lock(m_paStreamMutex);
            m_isPaused = true;
//            PaError err = Pa_StopStream(m_paStream.get());
//            if (err != paNoError)
//            {
//                isError = true;

//                SAL_DEBUG_STREAM_STATUS(std::string("Failed to pause stream: ") + Pa_GetErrorText(err))
//            }
        }
//        if (isError)
//        {
//            SAL_DEBUG_STREAM_STATUS("Buffering: pausing the steam failed: cannot pause the stream")

//            _resetStreamInfo();
//        }

        SAL_DEBUG_STREAM_STATUS("Buffering: pausing the stream done")
    }

    SAL_DEBUG_LOOP_UPDATE("Check if buffering done")
}

void Player::continuePlayingIfEnoughBuffering()
{
    if (_isPlaying() && m_isBuffering)
    {
//        bool isStartStreamFailed = false;
        {
            std::scoped_lock lock(m_paStreamMutex, m_queueOpenedFileMutex);
            for (const std::unique_ptr<AbstractAudioFile>& file : m_queueOpenedFile)
            {
                if (!file->isEnded())
                {
                    if (file->isEnoughBuffering())
                    {
                        SAL_DEBUG_STREAM_STATUS("Enough buffering, resume stream")

                        m_isBuffering = false;
                        m_isPaused = false;
//                        PaError err = Pa_StartStream(m_paStream.get());
//                        if (err != paNoError)
//                        {
//                            isStartStreamFailed = true;
                            
//                            SAL_DEBUG_STREAM_STATUS(std::string("Failed to remuse stream: ") + Pa_GetErrorText(err))
//                        }

                        streamEnoughBufferingCallback();

                        break; // Leaving the loop, no need try to restart another time PortAudio on the next file.
                    }
                }
            }
        }
//        if (isStartStreamFailed)
//        {
//            SAL_DEBUG_STREAM_STATUS("Enough buffering, resume stream failed: starting stream failed")

//            _resetStreamInfo();
//        }
    }
}

void Player::clearUnneededStream()
{
    while (m_queueOpenedFile.size() > 0)
    {
        if (!m_queueOpenedFile.at(0)->isEnded())
            break;
        else
        {
            SAL_DEBUG_STREAM_STATUS("Clearing unneeded streams")

            // Notify that the file ended.
            endStreamingFile(m_queueOpenedFile.at(0)->filePath());
            
            m_queueOpenedFile.erase(m_queueOpenedFile.cbegin());

            // Notify of the new file streaming.
            if (!m_queueOpenedFile.empty())
                startStreamingFile(m_queueOpenedFile.at(0)->filePath());
            
            // Restart checking file.
            m_doNotCheckFile = false;

            SAL_DEBUG_STREAM_STATUS("Clearing unneeded streams done")
        }
    }
}

void Player::recreateStream()
{
    if (!m_isPlaying)
        return;
    
    if (!m_paStream)
    {
        if (!m_queueOpenedFile.empty())
        {
            SAL_DEBUG_STREAM_STATUS("Recreating a new stream sink (the new file have not the same data informations)")

            if (!createStream())
            {
                _resetStreamInfo();
                m_isPlaying = false;
            }
            
            std::scoped_lock lock(m_paStreamMutex, m_queueOpenedFileMutex);
            PaError err = Pa_StartStream(m_paStream.get());
            if (err == paNoError)
            {
                SAL_DEBUG_STREAM_STATUS("Recreating a new stream sink done")

                m_isPlaying = true;
                
                // Call end stream callback.
                if (!m_queueOpenedFile.empty())
                    startStreamingFile(m_queueOpenedFile.at(0)->filePath());
            }
            else
            {
#ifndef NDEBUG
                SAL_DEBUG_STREAM_STATUS(std::string("Recreating a new stream sink failed: starting the stream failed: ") + Pa_GetErrorText(err))
#endif

                m_isPaused = false;
            }
        }
    }
}

void Player::closeStreamWhenNeeded()
{
    SAL_DEBUG_LOOP_UPDATE("Check if closing the stream")

    if (m_isClosingStreamTheStream)
    {
        SAL_DEBUG("Closing the stream")
        std::scoped_lock lock(m_paStreamMutex);
        resetStreamInfo();
        SAL_DEBUG("Closing the stream done")
    }

    SAL_DEBUG_LOOP_UPDATE("Check if closing the stream done")
}

void Player::updateStreamBuffer()
{
    SAL_DEBUG_LOOP_UPDATE("Reading data from files")

    for (std::unique_ptr<AbstractAudioFile>& audioFile : m_queueOpenedFile)
    {
        audioFile->readFromFile();
        audioFile->flush();
    }

    SAL_DEBUG_LOOP_UPDATE("Reading data from files done")
}

void Player::checkIfNoStream()
{
    if (m_isPlaying && m_queueFilePath.empty() && m_queueOpenedFile.empty())
    {
        m_isPlaying = false;
        m_isPaused = false;
    }
}

void Player::retrieveAvailableHostApi()
{
    const PaHostApiIndex hostApiCount = Pa_GetHostApiCount();
    for (int i = 0; i < hostApiCount; i++)
    {
        PaHostApiTypeId hostApiID = Pa_GetHostApiInfo(i)->type;
        m_availableHostApi.push_back(hostApiID);
    }
}

void Player::setBackendAudio(BackendAudio backend)
{
    if (backend == BackendAudio::INVALID_API || backend == BackendAudio::SYSTEM_DEFAULT)
    {
        m_backendAudio = getSystemDefaultBackendAudio();
        return;
    }

    m_backendAudio = backend;
}

BackendAudio Player::getSystemDefaultBackendAudio() const
{
    PaHostApiIndex hostApiIndex = Pa_GetDefaultHostApi();
    const PaHostApiInfo* hostApiInfo = Pa_GetHostApiInfo(hostApiIndex);
    return fromHostAPIToBackendEnum(hostApiInfo->type);
}

std::vector<BackendAudio> Player::availableBackendAudio() const
{
    std::vector<BackendAudio> backendsAudio;

    for (int id : m_availableHostApi)
    {
        const BackendAudio backend = fromHostAPIToBackendEnum(id);
        if (backend != BackendAudio::INVALID_API) {
            backendsAudio.push_back(backend);
        }
    }

    return backendsAudio;
}

BackendAudio Player::fromHostAPIToBackendEnum(int apiIndex) const
{
    switch(apiIndex)
    {
    case paDirectSound:
        return BackendAudio::DIRECT_SOUND;
    case paMME:
        return BackendAudio::MME;
    case paASIO:
        return BackendAudio::ASIO;
    case paWASAPI:
        return BackendAudio::WASAPI;
    case paWDMKS:
        return BackendAudio::WDMKS;
    case paOSS:
        return BackendAudio::OSS;
    case paALSA:
        return BackendAudio::ALSA;
    case paJACK:
        return BackendAudio::JACK;
    default:
        return BackendAudio::INVALID_API;
    }
}

int Player::fromBackendEnumToHostAPI(BackendAudio backend) const
{
    switch(backend)
    {
    case BackendAudio::DIRECT_SOUND:
        return paDirectSound;
    case BackendAudio::MME:
        return paMME;
    case BackendAudio::ASIO:
        return paASIO;
    case BackendAudio::WASAPI:
        return paWASAPI;
    case BackendAudio::WDMKS:
        return paWDMKS;
    case BackendAudio::OSS:
        return paOSS;
    case BackendAudio::ALSA:
        return paALSA;
    case BackendAudio::JACK:
        return paJACK;
    default:
        return paInDevelopment;
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
