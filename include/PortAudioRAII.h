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

    inline bool isInit() const;

private:
    bool m_isInit;
};

inline bool PortAudioRAII::isInit() const { return m_isInit; }
}

#endif // SIMPLE_AUDIO_LIBRARY_PORTAUDIORAII_H_