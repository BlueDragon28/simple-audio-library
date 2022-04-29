#include "PortAudioRAII.h"
#include <portaudio.h>

namespace SAL
{
PortAudioRAII::PortAudioRAII()
{
    Pa_Initialize();
}

PortAudioRAII::~PortAudioRAII()
{
    Pa_Terminate();
}
}