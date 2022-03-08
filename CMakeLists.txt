cmake_minimum_required(VERSION 3.8.0)
project(simple-audio-library
    LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include/)

set(PROJECT_SOURCES
    "src/RingBuffer.cpp")

# Default static library
add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")