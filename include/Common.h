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
}

#endif // SIMPLE_AUDIO_LIBRARY_COMMON_H_