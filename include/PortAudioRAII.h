#ifndef SIMPLE_AUDIO_LIBRARY_PORTAUDIORAII_H_
#define SIMPLE_AUDIO_LIBRARY_PORTAUDIORAII_H_

namespace SAL
{
class PortAudioRAII
{
    PortAudioRAII(const PortAudioRAII&) = delete;
public:
    PortAudioRAII();
    ~PortAudioRAII();
};
}

#endif // SIMPLE_AUDIO_LIBRARY_PORTAUDIORAII_H_