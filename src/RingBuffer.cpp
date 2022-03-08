#include "RingBuffer.h"
#include <cstring>

namespace SAL
{
RingBuffer::RingBuffer() :
    m_data(nullptr),
    m_size(0),
    m_tailPos(0),
    m_headPos(0),
    m_readAvailable(0),
    m_writeAvailable(0)
{}

RingBuffer::RingBuffer(size_t bufferSize) :
    m_data(nullptr),
    m_size(bufferSize),
    m_tailPos(0),
    m_headPos(0),
    m_readAvailable(0),
    m_writeAvailable(bufferSize)
{
    if (bufferSize > 0)
        m_data = new char[m_size];
}

RingBuffer::~RingBuffer()
{
    delete[] m_data;
}

/*
Resize the circular buffer to *bufferSize.
Remove remove any data inside the circular buffer.
*/
void RingBuffer::resizeBuffer(size_t bufferSize)
{
    delete[] m_data;
    m_data = nullptr;
    if (bufferSize > 0)
        m_data = new char[bufferSize];
}

/*
Read *size data inside the circular buffer
into the *buffer. Return how many bytes readed.
*/
size_t RingBuffer::read(char* buffer, size_t size)
{
    if (!buffer || !m_data || m_size == 0 || 
        size == 0 || m_readAvailable == 0)
        return 0;
    
    size_t readSize = size;
    if (size > m_readAvailable)
        readSize = m_readAvailable;
    
    if (readSize > m_size-m_tailPos)
    {
        int lenght = m_size-m_tailPos;
        memcpy(buffer, m_data+m_tailPos, lenght);
        memcpy(buffer+lenght, m_data, size-lenght);
    }
    else
        memcpy(buffer, m_data+m_tailPos, readSize);
    
    m_readAvailable -= readSize;
    m_writeAvailable += readSize;

    return readSize;
}

/*
Write *size of *buffer into the circular buffer.
Return how many bytes writed.
*/
size_t RingBuffer::write(const char* buffer, size_t size)
{
    if (!buffer || !m_data || m_size == 0 ||
        size == 0 || m_writeAvailable == 0)
        return 0;
    
    size_t writeSize = size;
    if (size > m_writeAvailable)
        writeSize = m_writeAvailable;
    
    if (writeSize > m_size-m_headPos)
    {
        int lenght = m_size-m_headPos;
        memcpy(m_data+m_headPos, buffer, lenght);
        memcpy(m_data, buffer+lenght, writeSize-lenght);
    }
    else
        memcpy(m_data+m_headPos, buffer, writeSize);
    
    m_readAvailable += writeSize;
    m_writeAvailable -= writeSize;

    return writeSize;
}

size_t RingBuffer::size() const
{
    return m_size;
}
}