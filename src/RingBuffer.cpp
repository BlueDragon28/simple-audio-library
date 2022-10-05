#include "RingBuffer.h"
#include <cstring>

// Define CLASS_NAME to have the name of the class.
const std::string CLASS_NAME = "RingBuffer";

namespace SAL
{
RingBuffer::RingBuffer() :
    m_data(nullptr),
    m_size(0),
    m_tailPos(0),
    m_headPos(0),
    m_writeAvailable(0)
{}

RingBuffer::RingBuffer(size_t bufferSize) :
    m_data(nullptr),
    m_size(bufferSize),
    m_tailPos(0),
    m_headPos(0),
    m_writeAvailable(bufferSize)
{
    if (bufferSize > 0)
        m_data = new char[m_size];
}

RingBuffer::RingBuffer(const RingBuffer& other)
{
    m_size = (size_t)other.m_size;
    m_tailPos = (size_t)other.m_tailPos;
    m_headPos = (size_t)other.m_headPos;
    m_writeAvailable = (size_t)other.m_writeAvailable;
    if (other.m_data && m_size > 0)
    {
        m_data = new char[m_size];
        memcpy(m_data, other.m_data, m_size);
    }
}

RingBuffer::~RingBuffer()
{
    delete[] m_data;
}

void RingBuffer::resizeBuffer(size_t bufferSize)
{
    std::scoped_lock lock(m_readMutex);
    delete[] m_data;
    m_data = nullptr;
    if (bufferSize > 0)
    {
        m_size = bufferSize;
        m_data = new char[m_size];
        m_tailPos = 0;
        m_headPos = 0;
        m_writeAvailable = (size_t)m_size;
    }
}

size_t RingBuffer::read(char* buffer, size_t size)
{
    std::scoped_lock lock(m_readMutex);
    // Check if there is data available to read.
    if (!buffer || !m_data || m_size == 0 || 
        size == 0 || m_writeAvailable == m_size)
        return 0;
    
    // Get the number in bytes of data to read.
    size_t readAvailable = m_size - m_writeAvailable;
    if (size > readAvailable)
        size = readAvailable;

    // Copy data into the output buffer.
    if (size > m_size-m_tailPos)
    {
        size_t lenght = m_size-m_tailPos;
        memcpy(buffer, m_data+m_tailPos, lenght);
        memcpy(buffer+lenght, m_data, size-lenght);
    }
    else
        memcpy(buffer, m_data+m_tailPos, size);
    
    // Move the tail position foward.
    m_tailPos = (m_tailPos + size) % m_size;
    m_writeAvailable += size;

    return size;
}

size_t RingBuffer::write(const char* buffer, size_t size)
{
    std::scoped_lock lock(m_readMutex);
    // Check if there is data to write.
    if (!buffer || !m_data || m_size == 0 ||
        size == 0 || m_writeAvailable == 0)
        return 0;
    
    // Get the number in bytes of data to write.
    if (size > m_writeAvailable)
        size = m_writeAvailable;

    // Copy data from input buffer into the ring buffer.
    if (size > m_size-m_headPos)
    {
        size_t lenght = m_size-m_headPos;
        memcpy(m_data+m_headPos, buffer, lenght);
        memcpy(m_data, buffer+lenght, size-lenght);
    }
    else
        memcpy(m_data+m_headPos, buffer, size);
    
    // Move the head position foward.
    m_headPos = (m_headPos + size) % m_size;
    m_writeAvailable -= size;

    return size;
}

void RingBuffer::clear()
{
    std::scoped_lock lock(m_readMutex);
    memset(m_data, 0, m_size);
    m_tailPos = 0;
    m_headPos = 0;
    m_writeAvailable = static_cast<size_t>(m_size);
}
}