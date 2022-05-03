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
    m_sleepTime(SLEEP_PAUSED)
{
    m_pa = std::unique_ptr<PortAudioRAII>(new PortAudioRAII());
    if (m_pa->isInit())
    {
        m_isInit = true;
        m_isRunning = true;
        m_player = std::unique_ptr<Player>(new Player());
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
    
    LoadFile loadFile = { filePath, clearQueue };
    m_events.push(EventType::OPEN_FILE, loadFile);
}

/*
Add a file into the playing list.
filePath: the file path to open.
clearQueue: stop file played and clear queue.
*/
void AudioPlayer::loop()
{
    if (!m_isInit)
        return;
    
    while (isRunning())
    {
        // Processing events.
        processEvents();

        // Wait time before next iteration.
        std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepTime));
    }
}

/*
Process the event send by the user.
*/
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

        // Quit
        case EventType::QUIT:
        {
            m_player->stop();
            m_isRunning = false;
        } break;
        
        // Invalid event.
        case EventType::INVALID:
        default:
        {
            continue;
        } break;
        }
    }
}
}