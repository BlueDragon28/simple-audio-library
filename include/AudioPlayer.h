#ifndef SIMPLE_AUDIO_LIBRARY_AUDIOPLAYER_H_
#define SIMPLE_AUDIO_LIBRARY_AUDIOPLAYER_H_

#include "Common.h"
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

private:
    bool m_isInit;
    bool m_isRunning;
    std::unique_ptr<PortAudioRAII> m_pa;
    std::unique_ptr<Player> m_player;

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