#ifndef SIMPLE_AUDIO_LIBRARY_PLAYER_H_
#define SIMPLE_AUDIO_LIBRARY_PLAYER_H_

#include "AbstractAudioFile.h"
#include <portaudio.h>
#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include <mutex>

namespace SAL
{
class CallbackInterface;

class Player
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
    play too.
    */
    void open(const char* filePath, bool clearQueue = false);
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
    Return true if the stream if playing.
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
    Return stream size in frames.
    */
    inline size_t streamSizeInFrames() const noexcept;

    /*
    Return stream pos in frames.
    */
    inline size_t streamPosInFrames() const noexcept;

    /*
    Return stream size in seconds.
    */
    inline size_t streamSize() const noexcept;

    /*
    Return stream pos in seconds.
    */
    inline size_t streamPos() const noexcept;

    /*
    Read audio data from file and push it
    into the ring buffer and push file from
    m_queueFilePath to m_queueOpenedFile.
    */
    void update();

    /*
    Set the pointer of the callback interface.
    */
    inline void setCallbackInterface(CallbackInterface* interface) noexcept;

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
    int checkFileFormat(const char* indentifiers, int size) const;

    /*
    Reset stream info.
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
    Static C callback use to make a bridge between
    PortAudio and this class.
    */
    static int staticPortAudioStreamCallback(
        const void* inputBuffer,
        void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags flags,
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

    // Next file to be opened after current file ended.
    std::vector<std::string> m_queueFilePath;
    std::mutex m_queueFilePathMutex;
    /*
    Current streamed file and the next file that have the same
    channels, bit depth, samplerate, sampleType.
    */
    std::vector<std::unique_ptr<AbstractAudioFile>> m_queueOpenedFile;
    mutable std::mutex m_queueOpenedFileMutex;

    // PortAudio stream interface.
    std::unique_ptr<PaStream, decltype(&Pa_CloseStream)> m_paStream;
    std::mutex m_paStreamMutex;

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

    // Prevent to loop trying to open a file if the file have a different stream information.
    bool m_doNotCheckFile;

    // Prevent streamEndCallback to call endStreamingFile callback if the stop method is called.
    bool m_isStopping;
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
inline void Player::setCallbackInterface(CallbackInterface* interface) noexcept
{
    m_callbackInterface = interface;
}

/*
Return stream size in frames.
*/
inline size_t Player::streamSizeInFrames() const noexcept
{
    std::scoped_lock lock(m_queueOpenedFileMutex);
    if (!m_queueOpenedFile.empty())
        return m_queueOpenedFile.at(0)->streamSize();
    else
        return 0;
}

/*
Return stream pos in frames.
*/
inline size_t Player::streamPosInFrames() const noexcept
{
    std::scoped_lock lock(m_queueOpenedFileMutex);
    if (!m_queueOpenedFile.empty())
        return m_queueOpenedFile.at(0)->streamPos();
    else
        return 0;
}

/*
Return stream size in seconds.
*/
inline size_t Player::streamSize() const noexcept
{
    std::scoped_lock lock(m_queueOpenedFileMutex);
    if (!m_queueOpenedFile.empty())
    {
        const std::unique_ptr<AbstractAudioFile>& audioFile =
            m_queueOpenedFile.at(0);
        return audioFile->streamSize() / audioFile->sampleRate();
    }
    else 
        return 0;
}

/*
Return stream pos in seconds.
*/
inline size_t Player::streamPos() const noexcept
{
    std::scoped_lock lock(m_queueOpenedFileMutex);
    if (!m_queueOpenedFile.empty())
    {
        const std::unique_ptr<AbstractAudioFile>& audioFile =
            m_queueOpenedFile.at(0);
        return audioFile->streamPos() / audioFile->sampleRate();
    }
    else
        return 0;
}
}

#endif // SIMPLE_AUDIO_LIBRARY_PLAYER_H_