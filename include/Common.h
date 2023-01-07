#ifndef SIMPLE_AUDIO_LIBRARY_COMMON_H_
#define SIMPLE_AUDIO_LIBRARY_COMMON_H_

#include "config.h"
#include <variant>
#include <string>

/*
Export defenition into the dll.
*/
#if defined(WIN32) && defined(BUILD_SHARED)
#ifdef SAL_BUILDING
#define SAL_EXPORT_DLL __declspec(dllexport)
#else
#define SAL_EXPORT_DLL __declspec(dllimport)
#endif
#else
#define SAL_EXPORT_DLL
#endif

namespace SAL
{
enum class SAL_EXPORT_DLL SampleType
{
    UNKNOWN,
    INT,
    UINT,
    FLOAT
};

enum class SAL_EXPORT_DLL EventType
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
    NEXT,
    REMOVE_ALL_BUT_CURRENT_PLAYBACK,
};

/*
Files formats types.
*/
enum SAL_EXPORT_DLL FileType
{
    UNKNOWN_FILE,
    WAVE,
    FLAC,
    SNDFILE,
};

enum class SAL_EXPORT_DLL TimeType
{
    FRAMES,
    SECONDS,
};

struct SAL_EXPORT_DLL LoadFile
{
    std::string filePath;
    bool clearQueue;
};

typedef std::variant<std::monostate,
                 int,
                 size_t,
                 LoadFile> SAL_EXPORT_DLL EventVariant;

struct SAL_EXPORT_DLL EventData
{
    EventType type;
    EventVariant data;
};

struct SAL_EXPORT_DLL FakeInt24
{
    uint8_t c[3];
};
}

#endif // SIMPLE_AUDIO_LIBRARY_COMMON_H_