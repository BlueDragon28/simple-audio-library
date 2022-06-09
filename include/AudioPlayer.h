#ifndef SIMPLE_AUDIO_LIBRARY_AUDIOPLAYER_H_
#define SIMPLE_AUDIO_LIBRARY_AUDIOPLAYER_H_

#include "Player.h"
#include "EventList.h"
#include "PortAudioRAII.h"
#include "CallbackInterface.h"
#include <string>
#include <memory>
#include <mutex>
#include <thread>

namespace SAL
{
/*
This is the main class of the simple-audio-libray.
This is the interface that control the library.
Initializing this class will start a main loop
inside another thread which you can communicate with
the methods inside this class.
*/
class AudioPlayer
{
    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer();
public:
    ~AudioPlayer();

    /*
    Create or return the existing instance
    of the AudioPlayer class. The instance
    is deleted by itself, you do not need
    to delete it.
    */
    static AudioPlayer* instance();

    /*
    Return the callbackInterface instance.
    */
    inline CallbackInterface& callback() noexcept;

    /*
    Destroy the the instance (if one is exinsting).
    */
    static void deinit();

    /*
    Return true if the PortAudio API is initialized.
    */
    inline bool isInit() const;

    /*
    Return true if the PortAudio API is initialized
    and if the processing loop is running.
    */
    inline bool isRunning() const;

    /*
    Return true if the player is playing.
    */
    bool isPlaying();

    /*
    Is files are ready to be playing or playing.
    */
    bool isReady();

    /*
    Return stream size in frames.
    */
    inline size_t streamSizeInFrames() const noexcept;

    /*
    Return stream pos in frames.
    */
    inline size_t streamPosInFrames() const noexcept;

    /*
    Return stream size in seconds.
    */
    inline size_t streamSize() const noexcept;

    /*
    Return stream pos in seconds.
    */
    inline size_t streamPos() const noexcept;

    /*
    Event to send to be processed.
    */

    /*
    Add a file into the playing list.
    filePath: the file path to open.
    clearQueue: stop file played and clear queue.
    */
    void open(const std::string& filePath, bool clearQueue = false);

    /*
    Checking if a file is readable by the simple-audio-library.
    */
    inline int isReadable(const std::string& filePath) const;

    /*
    Returning a list of strings containing the list of files formats supported by the simple-audio-library.
    */
    static std::vector<std::string> supportedFormats();

    /*
    Play the audio stream.
    */
    inline void play() noexcept;

    /*
    Pause the audio stream.
    */
    inline void pause() noexcept;

    /*
    Stop the audio stream and clear file queue.
    */
    inline void stop() noexcept;

    /*
    Seeking position in the audio stream.
    - pos: position in seconds if inSeconds is true,
    otherwise in frames.
    */
    inline void seek(size_t pos, bool inSeconds = true) noexcept;

    /*
    Stop the loop of AudioPlayer and stop playing audio stream.
    */
    inline void quit() noexcept;

private:
    /*
    This loop is executed in another thread
    and wait until the user want the api to stop.
    The loop process every event and send them to
    the Player and update the audio buffers.
    */
    void loop();

    /*
    Process the event send by the user.
    */
    void processEvents();

    /*
    The thread where the loop is executed.
    */
    std::thread m_loopThread;

    bool m_isInit;
    bool m_isRunning;
    // Time in milliseconds the loop will wait after each iteration.
    std::atomic<int> m_sleepTime;
    std::unique_ptr<PortAudioRAII> m_pa;
    std::unique_ptr<Player> m_player;

    /*
    An event list to store in an array
    all the event of the user to process them
    in the main loop.
    */
    EventList m_events;

    /*
    A callback list to store user defined callback.
    It also store a list of call event to be process
    by the main loop.
    */
    CallbackInterface m_callbackInterface;

    /*
    Only one instance of AudioPlayer is possible.
    To prevent the user to create multiple instance,
    the constructor is private and only a static class
    allow him to retrieve the instance.
    To prevent both the user and the smart_pointer to delete
    the instance, and the destructor of the object release the
    ptr of the smart_pointer if the user delete it.
    */
    static bool doNotReset;
    inline static void destroy(AudioPlayer* ptr);
    typedef std::unique_ptr<AudioPlayer, decltype(&AudioPlayer::destroy)> AudioPlayerPtr;
    static AudioPlayerPtr obj;
};

/*
Return true if the PortAudio API is initialized.
*/
inline bool AudioPlayer::isInit() const { return m_isInit; }

/*
Return true if the PortAudio API is initialized
and if the processing loop is running.
*/
inline bool AudioPlayer::isRunning() const
{
    if (isInit())
        return m_isRunning;
    else
        return false;
}

/*
Return stream size in frames.
*/
inline size_t AudioPlayer::streamSizeInFrames() const noexcept
{
    if (m_player)
        return m_player->streamSizeInFrames();
    return 0;
}

/*
Return stream pos in frames.
*/
inline size_t AudioPlayer::streamPosInFrames() const noexcept
{
    if (m_player)
        return m_player->streamPosInFrames();
    return 0;
}

/*
Return stream size in seconds.
*/
inline size_t AudioPlayer::streamSize() const noexcept
{
    if (m_player)
        return m_player->streamSize();
    return 0;
}

inline size_t AudioPlayer::streamPos() const noexcept
{
    if (m_player)
        return m_player->streamPos();
    return 0;
}

/*
Create or return the existing instance
of the AudioPlayer class. The instance
is deleted by itself, do not destroy it
by yourself, use the deinit method.
*/
inline AudioPlayer* AudioPlayer::instance()
{
    if (!obj)
        obj = AudioPlayerPtr(new AudioPlayer(), AudioPlayer::destroy);
    return obj.get();
}

/*
Destroy the the instance (if one is exinsting).
*/
inline void AudioPlayer::deinit() { obj.reset(); }

inline void AudioPlayer::destroy(AudioPlayer* ptr)
{
    doNotReset = true;
    delete ptr;
}

/*
Play the audio stream.
*/
inline void AudioPlayer::play() noexcept
{
    if (isRunning())
        m_events.push(EventType::PLAY);
}

/*
Pause the audio stream.
*/
inline void AudioPlayer::pause() noexcept
{
    if (isRunning())
        m_events.push(EventType::PAUSE);
}

/*
Stop the audio stream and clear file queue.
*/
inline void AudioPlayer::stop() noexcept
{
    if (isRunning())
        m_events.push(EventType::STOP);
}

/*
Seeking position in the audio stream.
- pos: position in seconds if inSeconds is true,
otherwise in frames.
*/
inline void AudioPlayer::seek(size_t pos, bool inSeconds) noexcept
{
    if (isReady())
    {
        if (inSeconds)
            m_events.push(EventType::SEEK_SECONDS, pos);
        else
            m_events.push(EventType::SEEK, pos);
    }
}

/*
Stop the loop of AudioPlayer and stop playing audio stream.
*/
inline void AudioPlayer::quit() noexcept
{
    if (isRunning())
        m_events.push(EventType::QUIT);
}

/*
Return the callbackInterface instance.
*/
inline CallbackInterface& AudioPlayer::callback() noexcept
{
    return m_callbackInterface;
}

/*
Checking if a file is readable by the simple-audio-library.
*/
inline int AudioPlayer::isReadable(const std::string& filePath) const
{
    return m_player->isReadable(filePath);
}
}

#endif // SIMPLE_AUDIO_LIBRARY_AUDIOPLAYER_H_