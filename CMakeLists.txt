cmake_minimum_required(VERSION 3.8.0)
project(simple-audio-library
    DESCRIPTION "A simple audio library written in C++"
    VERSION 0.1
    LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# PortAudio
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

# FLAC library
pkg_check_modules(FLAC++ REQUIRED flac++)

include_directories(include/)

set(PROJECT_SOURCES
    "src/RingBuffer.cpp"
    "src/AbstractAudioFile.cpp"
    "src/WaveAudioFile.cpp"
    "src/Player.cpp"
    "src/EventList.cpp"
    "src/PortAudioRAII.cpp"
    "src/AudioPlayer.cpp"
    "src/CallbackInterface.cpp"
    "src/FlacAudioFile.cpp")

# Default static library
add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES})
target_link_libraries(${PROJECT_NAME} ${PORTAUDIO_LIBRARIES} ${FLAC++_LIBRARIES})
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")