#include "AudioPlayer.h"
#include "Common.h"
#include "config.h"
#include <chrono>
#include <cstdint>
#include <ratio>
#include <iostream>

#define SLEEP_PLAYING 10
#define SLEEP_PAUSED 50

std::string SAL::AudioPlayer::description()
{
    return SAL_DESCRIPTION;
}

std::string SAL::AudioPlayer::version()
{
    return SAL_VERSION;
}

// Define CLASS_NAME to have the name of the class.
const std::string CLASS_NAME = "AudioPlayer";

// MACRO for redudent code of processEvents
#define SAL_DEBUG_PROCESS_EVENTS(type) \
    SAL_DEBUG_EVENTS(std::string("Processing ") + type + " event")

namespace SAL
{
AudioPlayer::AudioPlayerPtr AudioPlayer::obj =
    std::unique_ptr<AudioPlayer, decltype(&destroy)>(nullptr, AudioPlayer::destroy);
bool AudioPlayer::doNotReset = false;

AudioPlayer::AudioPlayer() :
    m_isInit(false),
    m_isRunning(false),
    m_sleepTime(SLEEP_PAUSED)
{
    SAL_DEBUG_SAL_INIT("Initializing SAL")

    // Initializing DebugLog
#ifdef DEBUG_LOG
    DebugLog::instance();
#endif

    // To work without errors, PortAudio must be initialized in the same thread that using it.
    m_loopThread = std::thread(&AudioPlayer::loop, this);

    std::unique_lock lock(m_initMutex);

    if (!m_isInit)
    {
        m_cvInit.wait(lock);
    }

    SAL_DEBUG_SAL_INIT("Initialization done")
}

AudioPlayer::~AudioPlayer()
{
    // Removing the ptr from the unique_ptr without
    // deleting the ptr if the user delete the ptr
    // by itself.
    if (!doNotReset)
        (void)obj.release();

    // Stopping the loop and wait for the thread to stop.
    m_isRunning = false;
    if (m_loopThread.joinable())
        m_loopThread.join();

    SAL_DEBUG_SAL_INIT("Deinitializing SAL")
}

void AudioPlayer::open(const std::string& filePath, bool clearQueue)
{
    SAL_DEBUG_EVENTS(std::string("Opening the file \"") + filePath + "\" with clear queue set to " + (clearQueue ? "true" : " false"))

    if (!isRunning())
    {
        SAL_DEBUG_EVENTS("Failed to open the file, main loop not running")
        return;
    }
    
    SAL_DEBUG_EVENTS("Adding the file into the event list with the type OPEN_FILE")

    // Push the event into the events list.
    LoadFile loadFile = { filePath, clearQueue };
    m_events.push(EventType::OPEN_FILE, loadFile);

    SAL_DEBUG_EVENTS("Adding the file into the event list done")
}

void AudioPlayer::open(const std::vector<std::string>& filesPath, bool clearQueue)
{
    // Using a loop to add each file into the event queue.
    for (const std::string& filePath : filesPath)
    {
        open(filePath, clearQueue);

        if (clearQueue)
        {
            clearQueue = false;
        }
    }
}

void AudioPlayer::initialize()
{
    SAL_DEBUG_SAL_INIT("Initialize audio system")

    std::unique_lock lock(m_initMutex);

    m_pa = std::unique_ptr<PortAudioRAII>(new PortAudioRAII());

    if (m_pa->isInit())
    {
        m_isRunning = true;
        m_player = std::unique_ptr<Player>(new Player());
        m_player->setCallbackInterface(&m_callbackInterface);

        // Enable the callback interface to get access of some getters of this class.
        m_callbackInterface.setIsReadyGetter(std::bind(&Player::isFileReady, m_player.get()));
    } else
    {
        m_isRunning = false;
    }

    m_isInit = true;
    m_cvInit.notify_one();

    SAL_DEBUG_SAL_INIT("Initialize audio system done!")
}

void AudioPlayer::loop()
{
    initialize();
    
    SAL_DEBUG_SAL_INIT("Starting main loop")

    while (isRunning())
    {
        SAL_DEBUG_LOOP_UPDATE("Main loop iteration")

        // Retrieve the time at the beguinning of the loop iteration.
        const auto timeBegin = std::chrono::system_clock::now();

        // Call the callbacks.
        m_callbackInterface.callback();

        // Processing events.
        processEvents();

        // Update stream buffers and push files in the
        // playing queue.
        m_player->update();

        // Getting the time the loop iteration take to process.
        const auto elapsedTime = 
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - timeBegin);

        // If the elapsed time is less than the time required between each iteration,
        // wait required time minus the elapsed time.
        if (elapsedTime.count() < m_sleepTime) {
            // Wait time before next iteration.
            std::this_thread::sleep_for(
                std::chrono::milliseconds(m_sleepTime - elapsedTime.count()));
        }
    }

    m_pa.reset();
}

void AudioPlayer::processEvents()
{
    SAL_DEBUG_LOOP_UPDATE("Processing pending events")

    while (m_events.containEvents())
    {
        EventData event = m_events.get();

        switch (event.type)
        {
        // Open a file
        case EventType::OPEN_FILE:
        {
            SAL_DEBUG_PROCESS_EVENTS("OPEN_FILE")

            LoadFile fileInfo;
            try
            {
                fileInfo = std::get<LoadFile>(event.data);
            }
            catch (const std::bad_variant_access&)
            {
                continue;
            }
            m_player->open(fileInfo.filePath, fileInfo.clearQueue);
//            m_player->update();
        } break;

        // Play
        case EventType::PLAY:
        {
            SAL_DEBUG_PROCESS_EVENTS("PLAY")

            m_player->play();
            m_sleepTime = SLEEP_PLAYING;
        } break;

        // Pause
        case EventType::PAUSE:
        {
            SAL_DEBUG_PROCESS_EVENTS("PAUSE")

            m_player->pause();
            m_sleepTime = SLEEP_PAUSED;
        } break;

        // Stop
        case EventType::STOP:
        {
            SAL_DEBUG_PROCESS_EVENTS("STOP")

            m_player->stop();
            m_sleepTime = SLEEP_PAUSED;
        } break;

        // Seek in seconds
        case EventType::SEEK:
        case EventType::SEEK_SECONDS:
        {
            SAL_DEBUG_PROCESS_EVENTS("SEEK/SEEK_SECONDS")

            size_t pos;
            try
            {
                pos = std::get<size_t>(event.data);
            }
            catch (const std::bad_variant_access&)
            {
                continue;
            }
            m_player->seek(pos, 
                event.type == EventType::SEEK_SECONDS ? true : false);
        } break;

        // Move to the next audio stream.
        case EventType::NEXT:
        {
            SAL_DEBUG_PROCESS_EVENTS("NEXT")

            m_player->next();
        } break;

        // Remove all but the current playing playback
        case EventType::REMOVE_ALL_BUT_CURRENT_PLAYBACK:
        {
            SAL_DEBUG_PROCESS_EVENTS("REMOVE_ALL_BUT_CURRENT_PLAYBACK")

            m_player->removeNotPlayedPlayback();
        } break;

        // Quit
        case EventType::QUIT:
        {
            SAL_DEBUG_PROCESS_EVENTS("QUIT")

            m_player->stop();
            m_isRunning = false;
        } break;
        
        // Invalid event.
        case EventType::WAIT_EVENT:
        case EventType::INVALID:
        default:
        {
            SAL_DEBUG_PROCESS_EVENTS("WAIT/INVALID")

            continue;
        } break;
        }
    }

    SAL_DEBUG_LOOP_UPDATE("Processing pending events done")
}

bool AudioPlayer::isPlaying(bool isWaiting)
{
    if (isRunning())
    {
        /*
        Push an wait event id into the event queue
        and wait until it disappear.
        */
        if (isWaiting)
        {
            waitEvent();
        }
        return m_player->isPlaying();
    }
    else
        return false;
}

bool AudioPlayer::isReady(bool isWaiting)
{
    if (isRunning())
    {
        /*
        Push an wait event id into the event queue
        and wait until it disappear.
        */
        if (isWaiting)
        {
            waitEvent();
        }
        return m_player->isFileReady();
    }
    else
        return false;
}

void AudioPlayer::waitEvent()
{
    /*
    Push an wait event id into the event queue
    and wait until it disappear.
    */
    int id = m_events.waitEvent();
    while (m_events.isWaitEventIDPresent(id))
    {
        std::this_thread::sleep_for(std::chrono::microseconds(m_sleepTime));
    }
}

std::string AudioPlayer::getAudioBackendName(BackendAudio backend)
{
    switch(backend)
    {
    case BackendAudio::DIRECT_SOUND:
        return u8"Direct Sound";
    case BackendAudio::MME:
        return u8"MME";
    case BackendAudio::ASIO:
        return u8"ASIO";
    case BackendAudio::WASAPI:
        return u8"WASAPI";
    case BackendAudio::OSS:
        return u8"OSS";
    case BackendAudio::ALSA:
        return u8"ALSA";
    case BackendAudio::JACK:
        return u8"JACK";
    case BackendAudio::SYSTEM_DEFAULT:
    case BackendAudio::INVALID_API:
    default:
        return u8"Invalid API";
    }
}
}
