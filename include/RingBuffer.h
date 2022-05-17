#ifndef SIMPLE_AUDIO_LIBRARY_RINGBUFFER_H_
#define SIMPLE_AUDIO_LIBRARY_RINGBUFFER_H_

#include <cstddef>
#include <atomic>
#include <mutex>

namespace SAL
{
/*
Circular buffer used to stream audio.
*/
class RingBuffer
{
public:
    RingBuffer();
    RingBuffer(size_t bufferSize);
    RingBuffer(const RingBuffer& other);
    ~RingBuffer();

    /*
    Read *size data inside the circular buffer
    into the *buffer. Return how many bytes readed.
    */
    size_t read(char* buffer, size_t size);
    /*
    Write *size of *buffer into the circular buffer.
    Return how many bytes writed.
    */
    size_t write(const char* buffer, size_t size);
    /*
    Resize the circular buffer to *bufferSize.
    Remove remove any data inside the circular buffer.
    This function is not thread safe, so be carefull when
    calling it.
    */
    void resizeBuffer(size_t bufferSize);

    inline size_t size() const noexcept;

    /*
    Return the size of data readable in the buffer.
    */
    inline size_t readable() const noexcept;

    /*
    Clearing the ring buffer of all data.
    */
    void clear();

private:
    char* m_data;
    std::atomic<size_t> m_size;
    std::atomic<size_t> m_tailPos;
    std::atomic<size_t> m_headPos;
    std::atomic<size_t> m_writeAvailable;

    /*
    Preventing the streamCallback and the update
    method to read, clear and resize the ring buffer 
    at the same time.
    */
    std::mutex m_readMutex;
};

inline size_t RingBuffer::readable() const noexcept
{
    return m_size - m_writeAvailable;
}

inline size_t RingBuffer::size() const noexcept
{
    return m_size;
}
}

#endif // SIMPLE_AUDIO_LIBRARY_RINGBUFFER_H_