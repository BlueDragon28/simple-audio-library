#ifndef SIMPLE_AUDIO_LIBRARY_PLAYER_H_
#define SIMPLE_AUDIO_LIBRARY_PLAYER_H_

#include "AbstractAudioFile.h"
#include "Common.h"
#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include <mutex>

struct PaStreamCallbackTimeInfo;
typedef void PaStream;

namespace SAL
{
class CallbackInterface;

/*
This class is doing all the job to communicate with PortAudio by
creating the stream and sending data to PortAudio.

It is managing the file queue, create new file stream and delete
them when necessary.

The stream is always readed has 32 bits floating point numbers.
*/
class SAL_EXPORT_DLL Player
{
    Player(const Player& other) = delete;
public:
    Player();
    ~Player();

    /*
    Add a new file into the queue and stream it when the queue is empty.
    filePath - path to the file.
    clearQueue - clear the queue and stop stream and use this file has
    current stream. If the previous file was playing, the new one will
    also play.
    */
    void open(const std::string& filePath, bool clearQueue = false);

    /*
    Checking if a file is readable by this library.
    */
    int isReadable(const std::string& filePath) const;

    /*
    Start paying if there is any stream to play.
    */
    void play();

    /*
    Pause the stream.
    */
    void pause();

    /*
    Stop playing and delete the queues.
    */
    void stop();

    /*
    Changing reading position in the audio stream.
    - pos: position in seconds if inSeconds is true,
    otherwise in frames.
    */
    inline void seek(size_t pos, bool inSeconds) noexcept;

    /*
    Move to the next audio stream (if available).
    The method check if the opened file array have
    at least two items or if the opened file array have 
    one item and the file path array have at least one item.
    */
    void next();

    /*
    Wrapper to the _isPlaying private method.
    The wrapper lock the opened file mutex.
    */
    bool isPlaying() const;

    /*
    Return true if the stream is paused and not stopped.
    */
    bool isPaused() const;

    /*
    Return true if any of the opened file
    are ready.
    */
    bool isFileReady() const;

    /*
    Return stream size of the raw stream (not necessarily in 32 bits float).
    timeType: choose between frames or seconds base time.
    Wrapper to the _streamSize method.
    */
    inline size_t streamSize(TimeType timeType) const noexcept;

    /*
    Return stream pos of the raw stream (not necessarily in 32 bits float).
    timeType: choose between frames or seconds base time.
    Wrapper to the _streamPos method.
    */
    inline size_t streamPos(TimeType timeType) const noexcept;

    /*
    Read audio data from file and push it
    into the ring buffer and push file from
    m_queueFilePath to m_queueOpenedFile.
    */
    void update();

    /*
    Set the pointer of the callback interface.
    */
    inline void setCallbackInterface(CallbackInterface* callbackInterface) noexcept;

    /*
    Remove all in queue files but keep only the current one.
    This is useful when enabling or disabling shuffle playback.
    */
    void removeNotPlayedPlayback();

    inline BackendAudio getBackendAudio() const;
    void setBackendAudio(BackendAudio backend);

    /*
    Convert host api enum to backend audio enum.
    */
    BackendAudio fromHostAPIToBackendEnum(int apiIndex) const;

    std::vector<BackendAudio> availableBackendAudio() const;

private:
    /*
    Remove ended file from m_queueOpenedFile and
    add file from m_queueFilePath if m_queueOpenedFile
    is empty or if they have the same stream info
    then the same file in m_queueOpenedFile.
    */
    void pushFile();

    /*
    Check what type is the file and opening it.
    */
    AbstractAudioFile* detectAndOpenFile(const std::string& filePath) const;

    /*
    Trying to detect the file format.
    */
    int checkFileFormat(const std::string& filepath) const;

    /*
    Reset stream info.
    */
    void _resetStreamInfo();
    /*
    Wrapper to the _resetStreamInfo.
    This method lock opened file mutex.
    */
    void resetStreamInfo();

    /*
    Check if the audio file (file) have the info than the
    currently played file.
    */
    bool checkStreamInfo(const AbstractAudioFile* const  file) const;

    /*
    Create the PaStream.
    */
    bool createStream();

    /*
    Pause the stream if buffering.
    */
    void pauseIfBuffering();

    /*
    Restart stream if buffering enough.
    */
    void continuePlayingIfEnoughBuffering();

    /*
    Remove the ended stream of m_queueOpenedFile.
    */
    void clearUnneededStream();

    /*
    Recreate the PortAudio stream for new files
    with different stream info.
    */
    void recreateStream();

    /*
    Close the PortAudio stream when asked.
    */
    void closeStreamWhenNeeded();

    /*
    Update audio stream buffer.
    Read from the audio files and push the data
    into the ring buffer.
    */
    void updateStreamBuffer();

    /*
    Set m_isPlaying to false if there is no audio file to stream.
    */
    void checkIfNoStream();

    /*
    Return true if the stream if playing.
    */
    bool _isPlaying() const;

    /*
    Return stream size of the raw stream (not necessarily in 32 bits float).
    timeType: choose between frames or seconds base time.
    */
    inline size_t _streamSize(TimeType timeType) const noexcept;

    /*
    Return stream pos of the raw stream (not necessarily in 32 bits float).
    timeType: choose between frames or seconds base time.
    */
    inline size_t _streamPos(TimeType timeType) const noexcept;

    /*
    Static C callback use to make a bridge between
    PortAudio and this class.
    */
    static int staticPortAudioStreamCallback(
        const void* inputBuffer,
        void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        unsigned long flags,
        void* data);
    static void staticPortAudioEndStream(void* data);
    
    /*
    Stream callback used to collect audio stream
    from the audio file interface and sending it to
    PortAudio.
    */
    int streamCallback(
        const void* inputBuffer,
        void* outputBuffer,
        unsigned long framesPerBuffer);

    /*
    When the stream reach end, this member function
    is called.
    */
    void streamEndCallback();

    /*
    Call the start file callback.
    */
    void startStreamingFile(const std::string& filePath);

    /*
    Call the end file callback.
    */
    void endStreamingFile(const std::string& filePath);

    /*
    Call the stream position change in frames.
    */
    void streamPosChangeInFrames(size_t streamPos);

    /*
    Call stream position (in seconds) callback if the position (in seconds)
    changed since the last callback.
    */
    void streamPosChangeCallback();

    /*
    Called when the stream is pausing.
    */
    void streamPausedCallback();

    /*
    Called when the stream is playing (or resuming).
    */
    void streamPlayingCallback();

    /*
    Called when the stream is stopping.
    */
    void streamStoppingCallback();

    /*
    Called when the stream is buffering.
    */
    void streamBufferingCallback();

    /*
    Called when the stream have enough buffering.
    */
    void streamEnoughBufferingCallback();

    /*
    Retrieve the list of backend API available.
    */
    void retrieveAvailableHostApi();

    BackendAudio getSystemDefaultBackendAudio() const;

    /*
    Convert backend audio enum to host api enum.
    */
    int fromBackendEnumToHostAPI(BackendAudio backend) const;

    // Next file to be opened after current file ended.
    std::vector<std::string> m_queueFilePath;
    std::mutex m_queueFilePathMutex;

    /*
    Current streamed file and the next file that have the same
    channels and samplerate.
    */
    std::vector<std::unique_ptr<AbstractAudioFile>> m_queueOpenedFile;
    mutable std::mutex m_queueOpenedFileMutex;

    // PortAudio stream interface.
    static int _destroyStream(PaStream* stream);
    std::unique_ptr<PaStream, decltype(&_destroyStream)> m_paStream;
    std::atomic<BackendAudio> m_backendAudio;
    std::mutex m_paStreamMutex;
    std::atomic<bool> m_isClosingStreamTheStream; // When a stream stop, it ask to close the stream.

    // If the stream is playing or not.
    std::atomic<bool> m_isPlaying;
    std::atomic<bool> m_isPaused;
    std::atomic<bool> m_isBuffering;

    // Maximum of same stream in the m_queueOpenedFile queue.
    std::atomic<int> m_maxInStreamQueue;

    /*
    Stream info.
    */
    std::atomic<short> m_numChannels;
    std::atomic<size_t> m_sampleRate;
    std::atomic<int> m_bytesPerSample;
    std::atomic<SampleType> m_sampleType;

    /*
    Pointer to the callback interface.
    */
    CallbackInterface* m_callbackInterface;

    /*
    Last position in seconds registered by stream pos callback
    */
    size_t m_streamPosLastCallback;

    // Prevent infinity loop trying to open a file if the file have a different stream information.
    bool m_doNotCheckFile;

    // Prevent streamEndCallback to call endStreamingFile callback if the stop method is called.
    bool m_isStopping;

    // List of available backend audio.
    std::vector<int> m_availableHostApi;
};

/*
Changing reading position in the audio stream.
- pos: position in seconds if inSeconds is true,
otherwise in frames.
*/
inline void Player::seek(size_t pos, bool inSeconds) noexcept
{
    std::scoped_lock lock(m_queueOpenedFileMutex);
    if (!m_queueOpenedFile.empty())
    {
        if (inSeconds)
            m_queueOpenedFile.at(0)->seekInSeconds(pos);
        else
            m_queueOpenedFile.at(0)->seek(pos);
    }
}

/*
Set the pointer of the callback interface.
*/
inline void Player::setCallbackInterface(CallbackInterface* callbackInterface) noexcept
{
    m_callbackInterface = callbackInterface;
}

/*
Return stream size of the raw stream (not necessarily in 32 bits float).
timeType: choose between frames or seconds base time.
Wrapper to the _streamSize method.
*/
inline size_t Player::streamSize(TimeType timeType) const noexcept
{
    std::scoped_lock lock(m_queueOpenedFileMutex);
    return _streamSize(timeType);
}

/*
Return stream size of the raw stream (not necessarily in 32 bits float).
timeType: choose between frames or seconds base time.
*/
inline size_t Player::_streamSize(TimeType timeType) const noexcept
{
    if (!m_queueOpenedFile.empty())
    {
        const std::unique_ptr<AbstractAudioFile>& audioFile =
            m_queueOpenedFile.at(0);
        if (timeType == TimeType::FRAMES)
            return m_queueOpenedFile.at(0)->streamSize();
        else
            return audioFile->streamSize() / audioFile->sampleRate();
    }
    else 
        return 0;
}

/*
Return stream pos of the raw stream (not necessarily in 32 bits float).
timeType: choose between frames or seconds base time.
Wrapper to the _streamPos method.
*/
inline size_t Player::streamPos(TimeType timeType) const noexcept
{
    std::scoped_lock lock(m_queueOpenedFileMutex);
    return _streamPos(timeType);
}

/*
Return stream pos of the raw stream (not necessarily in 32 bits float).
timeType: choose between frames or seconds base time.
*/
inline size_t Player::_streamPos(TimeType timeType) const noexcept
{
    if (!m_queueOpenedFile.empty())
    {
        const std::unique_ptr<AbstractAudioFile>& audioFile =
            m_queueOpenedFile.at(0);
        if (timeType == TimeType::FRAMES)
            return m_queueOpenedFile.at(0)->streamPos();
        else
            return audioFile->streamPos() / audioFile->sampleRate();
    }
    else
        return 0;
}

inline BackendAudio Player::getBackendAudio() const
{
    return m_backendAudio;
}
}

#endif // SIMPLE_AUDIO_LIBRARY_PLAYER_H_
