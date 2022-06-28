#ifndef SIMPLE_AUDIO_LIBRARY_COMMON_H_
#define SIMPLE_AUDIO_LIBRARY_COMMON_H_

#include <variant>
#include <string>

namespace SAL
{
enum class SampleType
{
    UNKNOWN,
    INT,
    UINT,
    FLOAT
};

enum class EventType
{
    INVALID,
    OPEN_FILE,
    PLAY,
    PAUSE,
    STOP,
    QUIT,
    WAIT_EVENT,
    SEEK,
    SEEK_SECONDS,
};

/*
Files formats types.
*/
enum FileType
{
    UNKNOWN_FILE,
    WAVE,
    FLAC,
    SNDFILE,
};

enum class TimeType
{
    FRAMES,
    SECONDS,
};

struct LoadFile
{
    std::string filePath;
    bool clearQueue;
};

typedef std::variant<std::monostate,
                 int,
                 size_t,
                 LoadFile> EventVariant;

struct EventData
{
    EventType type;
    EventVariant data;
};

struct FakeInt24
{
    char c[3];
};
}

#endif // SIMPLE_AUDIO_LIBRARY_COMMON_H_