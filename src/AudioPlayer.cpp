#include "AudioPlayer.h"
#include "Common.h"
#include <chrono>

#define SLEEP_PLAYING 10
#define SLEEP_PAUSED 50

namespace SAL
{
AudioPlayer::AudioPlayerPtr AudioPlayer::obj =
    std::unique_ptr<AudioPlayer, decltype(&destroy)>(nullptr, AudioPlayer::destroy);
bool AudioPlayer::doNotReset = false;

AudioPlayer::AudioPlayer() :
    m_isInit(false),
    m_isRunning(false),
    m_sleepTime(SLEEP_PAUSED),
    m_debugLog(DebugLog::instance())
{
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
}

AudioPlayer::~AudioPlayer()
{
    // Removing the ptr from the unique_ptr without
    // deleting the ptr if the user delete the ptr
    // by itself.
    if (!doNotReset)
        obj.release();

    // Stopping the loop and wait for the thread to stop.
    m_isRunning = false;
    if (m_loopThread.joinable())
        m_loopThread.join();
}

void AudioPlayer::open(const std::string& filePath, bool clearQueue)
{
    if (!isRunning())
        return;
    
    // Push the event into the events list.
    LoadFile loadFile = { filePath, clearQueue };
    m_events.push(EventType::OPEN_FILE, loadFile);
}

void AudioPlayer::loop()
{
    if (!m_isInit)
        return;
    
    while (isRunning())
    {
        // Call the callbacks.
        m_callbackInterface.callback();

        // Processing events.
        processEvents();

        // Update stream buffers and push files in the
        // playing queue.
        m_player->update();

        // Flush debug log into the log file.
        m_debugLog->flush();

        // Wait time before next iteration.
        std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepTime));
    }
}

void AudioPlayer::processEvents()
{
    while (m_events.containEvents())
    {
        EventData event = m_events.get();

        switch (event.type)
        {
        // Open a file
        case EventType::OPEN_FILE:
        {
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
            m_player->play();
            m_sleepTime = SLEEP_PLAYING;
        } break;

        // Pause
        case EventType::PAUSE:
        {
            m_player->pause();
            m_sleepTime = SLEEP_PAUSED;
        } break;

        // Stop
        case EventType::STOP:
        {
            m_player->stop();
            m_sleepTime = SLEEP_PAUSED;
        } break;

        // Seek in seconds
        case EventType::SEEK:
        case EventType::SEEK_SECONDS:
        {
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
            m_player->next();
        } break;

        // Quit
        case EventType::QUIT:
        {
            m_player->stop();
            m_isRunning = false;
        } break;
        
        // Invalid event.
        case EventType::WAIT_EVENT:
        case EventType::INVALID:
        default:
        {
            continue;
        } break;
        }
    }
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
