#ifndef SIMPLE_AUDIO_LIBRARY_AUDIOPLAYER_H_
#define SIMPLE_AUDIO_LIBRARY_AUDIOPLAYER_H_

#include "Player.h"
#include "EventList.h"
#include "PortAudioRAII.h"
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
    Event to send to be processed.
    */

    /*
    Add a file into the playing list.
    filePath: the file path to open.
    clearQueue: stop file played and clear queue.
    */
    void open(const std::string& filePath, bool clearQueue);

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
    std::unique_ptr<PortAudioRAII> m_pa;
    std::unique_ptr<Player> m_player;

    /*
    An event list to store in an array
    all the event of the user to process them
    in the main loop.
    */
    EventList m_events;

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
}

#endif // SIMPLE_AUDIO_LIBRARY_AUDIOPLAYER_H_