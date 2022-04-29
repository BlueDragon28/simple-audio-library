#include "AudioPlayer.h"
#include <chrono>

namespace SAL
{
AudioPlayer::AudioPlayerPtr AudioPlayer::obj =
    std::unique_ptr<AudioPlayer, decltype(&destroy)>(nullptr, AudioPlayer::destroy);
bool AudioPlayer::doNotReset = false;

AudioPlayer::AudioPlayer() :
    m_isInit(false),
    m_isRunning(false)
{
    m_pa = std::unique_ptr<PortAudioRAII>(new PortAudioRAII());
    if (m_pa->isInit())
    {
        m_isInit = true;
        m_isRunning = false;
        m_player = std::unique_ptr<Player>(new Player());
    }
}

AudioPlayer::~AudioPlayer()
{
    // Removing the ptr from the unique_ptr without
    // deleting the ptr if the user delete the ptr
    // by itself.
    if (!doNotReset)
        obj.release();
}
}
