#include "PortAudioRAII.h"
#include <portaudio.h>

namespace SAL
{
PortAudioRAII::PortAudioRAII() :
    m_isInit(false)
{
    PaError err = Pa_Initialize();
    if (err == paNoError)
        m_isInit = true;
}

PortAudioRAII::~PortAudioRAII()
{
    Pa_Terminate();
}
}