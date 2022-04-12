#include "Player.h"
#include <filesystem>

namespace SAL
{
Player::Player() :
    // PortAudio stream interface.
    m_paStream(nullptr, Pa_CloseStream),

    // If the stream is playing or not.
    m_isPlaying(false),

    // Maximum of same stream in the m_queueOpenedFile queue.
    m_maxInStreamQueue(2)
{}

Player::~Player()
{}

/*
Add a new file into the queue and stream it when the queue is empty.
filePath - path to the file.
clearQueue - clear the queue and stop stream and use this file has
current stream. If the previous file was playing, the new one will
play too.
*/
void Player::open(const char* filePath, bool clearQueue)
{
    open(std::string(filePath), clearQueue);
}

void Player::open(const std::string& filePath, bool clearQueue)
{
    if (filePath.empty())
        return;
    
    bool isCurrentPlaying = isPlaying();
    stop();

    bool isExisting = std::filesystem::exists(filePath);
    m_queueFilePath.push_back(filePath);

    pushFile();
}

void Player::play()
{}

void Player::pause()
{}

void Player::stop()
{}

bool Player::isPlaying() const
{
    if (m_isPlaying && !m_queueOpenedFile.empty())
        return true;
    else
        return false;
}

void Player::pushFile()
{}
}