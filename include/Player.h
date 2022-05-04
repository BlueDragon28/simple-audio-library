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
    size_t streamSizeInFrames() const;

    /*
    Return stream pos in frames.
    */
    size_t streamPosInFrames() const;

    /*
    Read audio data from file and push it
    into the ring buffer and push file from
    m_queueFilePath to m_queueOpenedFile.
    */
    void update();

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
    AbstractAudioFile* detectAndOpenFile(const std::string& filePath);

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

    // Maximum of same stream in the m_queueOpenedFile queue.
    std::atomic<int> m_maxInStreamQueue;

    /*
    Stream info.
    */
    std::atomic<short> m_numChannels;
    std::atomic<size_t> m_sampleRate;
    std::atomic<int> m_bytesPerSample;
    std::atomic<SampleType> m_sampleType;
    std::atomic<bool> m_isPaused;
};
}

#endif // SIMPLE_AUDIO_LIBRARY_PLAYER_H_