#include "AudioPlayer.h"
#include "Common.h"
#include <chrono>

#define SLEEP_PLAYING 10
#define SLEEP_PAUSED 50

// Define CLASS_NAME to have the name of the class.
const std::string CLASS_NAME = "AudioPlayer";

// MACRO for redudent code of processEvents
#define SAL_DEBUG_PROCESS_EVENTS(type) \
    SAL_DEBUG(std::string("Processing ") + type + " event")

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
    SAL_DEBUG("Initializing SAL")

    // Initializing DebugLog
#ifndef NDEBUG
    DebugLog::instance();
#endif

    m_pa = std::unique_ptr<PortAudioRAII>(new PortAudioRAII());
    if (m_pa->isInit())
    {
        m_isInit = true;
        m_isRunning = true;
        m_player = std::unique_ptr<Player>(new Player());
        m_player->setCallbackInterface(&m_callbackInterface);

        // Enable the callback interface to get access of some getters of this class.
        m_callbackInterface.setIsReadyGetter(std::bind(&Player::isFileReady, m_player.get()));

        m_loopThread = std::thread(&AudioPlayer::loop, this);
    }

    SAL_DEBUG("Initialization done")
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

    SAL_DEBUG("Deinitializing SAL")
}

void AudioPlayer::open(const std::string& filePath, bool clearQueue)
{
    SAL_DEBUG(std::string("Opening the file \"") + filePath + "\" with clear queue set to " + (clearQueue ? "true" : " false"))

    if (!isRunning())
    {
        SAL_DEBUG("Failed to open the file, main loop not running")
        return;
    }
    
    SAL_DEBUG("Adding the file into the event list with the type OPEN_FILE")

    // Push the event into the events list.
    LoadFile loadFile = { filePath, clearQueue };
    m_events.push(EventType::OPEN_FILE, loadFile);

    SAL_DEBUG("Adding the file into the event list done")
}

void AudioPlayer::loop()
{
    if (!m_isInit)
        return;
    
    SAL_DEBUG("Starting main loop")

    while (isRunning())
    {
        SAL_DEBUG("Main loop iteration")

        // Call the callbacks.
        m_callbackInterface.callback();

        // Processing events.
        processEvents();

        // Update stream buffers and push files in the
        // playing queue.
        m_player->update();

        // Wait time before next iteration.
        std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepTime));
    }
}

void AudioPlayer::processEvents()
{
    SAL_DEBUG("Processing pending events")

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

    SAL_DEBUG("Processing pending events done")
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
}
