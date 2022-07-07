cmake_minimum_required(VERSION 3.8.0)
project(simple-audio-library
    DESCRIPTION "A simple audio library written in C++"
    VERSION 0.1
    LANGUAGES CXX)

# Options
# Use the libsndfile to read audio files.
option(USE_WAVE "Use the built-in WAVE file reader. It will be used to open WAVE file instead of libsndfile (if on)." ON)
option(USE_LIBSNDFILE "Use the libsndfile backend to read audio files." OFF)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# PortAudio
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

# FLAC library
pkg_check_modules(FLAC++ REQUIRED flac++)

# libsndfile library
if (USE_LIBSNDFILE)
    pkg_check_modules(LIBSNDFILE REQUIRED sndfile)
    set(LIBSNDFILE_PKG_LINK "-lsndfile")
endif()

# Configure the config.h file.
configure_file(include/config.h.in ${CMAKE_BINARY_DIR}/include/config.h)

include_directories(include/ ${CMAKE_BINARY_DIR}/include/)

set(PROJECT_HEADERS
    "include/AbstractAudioFile.h"
    "include/AudioPlayer.h"
    "include/CallbackInterface.h"
    "include/Common.h"
    "include/EventList.h"
    "include/FlacAudioFile.h"
    "include/Player.h"
    "include/PortAudioRAII.h"
    "include/RingBuffer.h"
    "${CMAKE_BINARY_DIR}/include/config.h")

set(PROJECT_SOURCES
    "src/RingBuffer.cpp"
    "src/AbstractAudioFile.cpp"
    "src/Player.cpp"
    "src/EventList.cpp"
    "src/PortAudioRAII.cpp"
    "src/AudioPlayer.cpp"
    "src/CallbackInterface.cpp"
    "src/FlacAudioFile.cpp")

# Compile the WAVE file is it is used.
if (USE_WAVE)
    set(PROJECT_HEADERS 
        "${PROJECT_HEADERS}"
        "include/WaveAudioFile.h")

    set(PROJECT_SOURCES 
        "${PROJECT_SOURCES}"
        "src/WaveAudioFile.cpp")
endif()

# Compile the libsndfile if it is used.
if (USE_LIBSNDFILE)
    set(PROJECT_HEADERS 
        "${PROJECT_HEADERS}"
        "include/SndAudioFile.h")
    
    set(PROJECT_SOURCES 
        "${PROJECT_SOURCES}"
        "src/SndAudioFile.cpp")
endif()

# Default static library
add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES})
target_link_libraries(${PROJECT_NAME} ${PORTAUDIO_LIBRARIES} ${FLAC++_LIBRARIES} ${LIBSNDFILE_LIBRARIES})
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# If CMAKE_INSTALL_LIBDIR is not define, set it to libdir
if (NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR lib)
endif()

# Generate the pkgconfig file
configure_file(
    "${CMAKE_SOURCE_DIR}/simple-audio-library_static.pc.in"
    "${CMAKE_BINARY_DIR}/simple-audio-library_static.pc"
    @ONLY)

# Add the install target
# Copy the library
if (WIN32)
    install(
        TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION bin)
else ()
    install(
        TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif ()

# Copy the install directory
install (
    FILES ${PROJECT_HEADERS}
    DESTINATION include/simple-audio-library)

# Copy the pkgconfig file
install (
    FILES ${CMAKE_BINARY_DIR}/simple-audio-library_static.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)