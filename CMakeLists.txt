cmake_minimum_required(VERSION 3.8.0)
project(simple-audio-library
    DESCRIPTION "A simple audio library written in C++"
    VERSION 0.2
    LANGUAGES CXX)

# Options
#build shared libs
option(BUILD_SHARED "Build the shared library instead of the static one." OFF)

# Use the libsndfile to read audio files.
option(USE_WAVE "Use the built-in WAVE file reader. It will be used to open WAVE file instead of libsndfile (if on)." ON)
option(USE_FLAC "Use the libFLAC++ backend to read and decode FLAC file. It will be used to open FLAC file instead of libsndfile (if on)." ON)
option(USE_LIBSNDFILE "Use the libsndfile backend to read audio files." OFF)
option(DEBUG_LOG "Enable debug logs (resource intensive)" OFF)

# Enable and disable DEBUG_LOG information, only available when DEBUG_LOG is enable
if (DEBUG_LOG)
    option(LOG_SAL_INIT "Log informations on initialization and deinitialization of SAL" ON)
    option(LOG_EVENTS "Log informations on events" ON)
    option(LOG_STREAM_STATUS "Log informations on the status of the stream" ON)
    option(LOG_OPEN_FILE "Log informations when loading and closing a file" ON)
    option(LOG_READ_FILE "Log informations on reading from file (resource intensive)" OFF)
    option(LOG_READ_STREAM "Log informations on reading stream (resource intensive)" OFF)
    option(LOG_LOOP_UPDATE "Log informations when the main loop is running (resource intensive)" OFF)
endif()

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# On windows, include cmake prefix path and include directories of the libraries
if (WIN32)
    # Find PortAudio
    if (
        (EXISTS "${CMAKE_SOURCE_DIR}/dependencies/portaudio/include/portaudio.h") AND
        ((EXISTS "${CMAKE_SOURCE_DIR}/dependencies/portaudio/lib/portaudio_x64.lib") OR (EXISTS "${CMAKE_SOURCE_DIR}/dependencies/portaudio/lib/portaudio.lib")))
        # Include directory of PortAudio
        include_directories(${CMAKE_SOURCE_DIR}/dependencies/portaudio/include)

        if (EXISTS "${CMAKE_SOURCE_DIR}/dependencies/portaudio/lib/portaudio_x64.lib")
            set(PORTAUDIO_DLL "${CMAKE_SOURCE_DIR}/dependencies/portaudio/lib/portaudio_x64.lib")
        else()
            set(PORTAUDIO_DLL "${CMAKE_SOURCE_DIR}/dependencies/portaudio/lib/portaudio.lib")
        endif()
    else()
        message(FATAL_ERROR "Cannot find portaudio")
    endif()

    # Find FLAC++
    if (USE_FLAC)
        if (
            (EXISTS "${CMAKE_SOURCE_DIR}/dependencies/flac/include/FLAC++/decoder.h") AND
            (EXISTS "${CMAKE_SOURCE_DIR}/dependencies/flac/lib/FLAC++.lib"))
            # Include directory of FLAC++ library
            include_directories("${CMAKE_SOURCE_DIR}/dependencies/flac/include/")
            # Set the FLAC++ library path.
            set(FLAC_LIBRARY 
                "${CMAKE_SOURCE_DIR}/dependencies/flac/lib/FLAC++.lib")
        else ()
            message(FATAL_ERROR "Cannot find FLAC++ library")
        endif()
    endif()

    # Find LIBSNDFILE
    if (USE_LIBSNDFILE)
        set(CMAKE_PREFIX_PATH 
            "${CMAKE_PREFIX_PATH}"
            "${CMAKE_SOURCE_DIR}/dependencies/libsndfile/cmake")
        find_package(SndFile REQUIRED)
        set(LIBSNDFILE_LIBRARY 
            ${SndFile_LIBRARIES})
    endif()
endif()

# Check for package with pkgconfig only on linux.
if (UNIX)
    # PortAudio
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

    # FLAC library
    if (USE_FLAC)
        pkg_check_modules(FLAC++ REQUIRED flac++)
        if (NOT BUILD_SHARED)
            set(LIBFLAC_PKG_LINK "-lFLAC++")
        endif()
    endif()

    # libsndfile library
    if (USE_LIBSNDFILE)
        pkg_check_modules(LIBSNDFILE REQUIRED sndfile)
        if (NOT BUILD_SHARED)
            set(LIBSNDFILE_PKG_LINK "-lsndfile")
        endif()
    endif()
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
    "include/Player.h"
    "include/PortAudioRAII.h"
    "include/RingBuffer.h"
    "include/DebugLog.h"
    "include/UTFConvertion.h"
    "${CMAKE_BINARY_DIR}/include/config.h")

set(PROJECT_SOURCES
    "src/RingBuffer.cpp"
    "src/AbstractAudioFile.cpp"
    "src/Player.cpp"
    "src/EventList.cpp"
    "src/PortAudioRAII.cpp"
    "src/AudioPlayer.cpp"
    "src/CallbackInterface.cpp"
    "src/DebugLog.cpp"
    "src/UTFConvertion.cpp")

# Compile the WAVE file if it is used.
if (USE_WAVE)
    set(PROJECT_SOURCES 
        "${PROJECT_SOURCES}"
        "src/WaveAudioFile.cpp"
        "src/WaveAudioFile.h")
endif()

# Compile the FLAC file if it is used.
if (USE_FLAC)
    set(PROJECT_HEADERS 
        "${PROJECT_HEADERS}"
        "include/FlacAudioFile.h")
    
    set(PROJECT_SOURCES 
        "${PROJECT_SOURCES}"
        "src/FlacAudioFile.cpp")
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

# Check if compile library has static or shared
if (BUILD_SHARED)
    set(COMPILE_TYPE SHARED)
else()
    set(COMPILE_TYPE STATIC)
endif()
# Build library.
add_library(${PROJECT_NAME} ${COMPILE_TYPE} ${PROJECT_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# On linux link the libraries with the variable made by pkgconfig.
if (UNIX)
    target_link_libraries(${PROJECT_NAME} ${PORTAUDIO_LIBRARIES} ${FLAC++_LIBRARIES} ${LIBSNDFILE_LIBRARIES})
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} 
        ${PORTAUDIO_DLL}
        "${FLAC_LIBRARY}"
        ${LIBSNDFILE_LIBRARY})
    if (BUILD_SHARED)
        target_compile_definitions(${PROJECT_NAME} PUBLIC SAL_BUILDING)
    endif()
endif()

# If CMAKE_INSTALL_LIBDIR is not define, set it to libdir
if (NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR lib)
endif()

# Generate the pkgconfig file (only on linux)
if (UNIX)
    configure_file(
        "${CMAKE_SOURCE_DIR}/simple-audio-library.pc.in"
        "${CMAKE_BINARY_DIR}/simple-audio-library.pc"
        @ONLY)
endif()

# Add the install target
# Copy the library
if (WIN32)
    install(
        TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION bin
        RUNTIME DESTINATION bin)
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

# Copy the pkgconfig file (only on linux)
if (UNIX)
    install (
        FILES ${CMAKE_BINARY_DIR}/simple-audio-library.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()
