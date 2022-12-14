# simple-audio-library

**simple-audio-library** abbreviated **SAL** is a simple *C++17 library* for *playing audio files*.

## Features

SAL support playback and seeking (moving the playing position of a stream) of audio files. 
It has a playlist (any number of files can be added to it) and the files will be played one after the others. 
If each audio files have the same *audio characteristic* (the same *sample rate* and the same *number of channels*), they will be played after each others *without interruption*.

The library has a main **loop** running in a **separate thread**, the call like open a file, play or pause will not stop the current thread where they are called.

SAL use the **PortAudio** library to play the audio stream.
All the audio files read are converted to **32 bits float stream**.
The library has a built-in **WAVE** file support.
It uses the **FLAC++** library to stream **FLAC** files and the **libsndfile** library (which support a lot of audio files format) for any overs files formats. 

## Compilation

The SAL use the **CMake** build generator. It's only required dependency is the [PortAudio](https://github.com/PortAudio/portaudio) library. There are options to enable or disable dependencies.

**Options** :
- **USE_WAVE** enabled by default: compile the built-in WAVE file reader. It will be used instead of the libsndfile library to play WAVE files.
- **USE_FLAC** enabled by default: compile the FLAC support. Depend on the [FLAC](https://github.com/xiph/flac) library. It will be used instead of the libsndfile library to play FLAC files.
- **USE_LIBSNDFILE** disabled by default: compile the libsndfile support. Depend on the [libsndfile](https://github.com/libsndfile/libsndfile) library.

To enable an option, you can use either the CMake GUI tool or by command line options.
To enable an option using command line, prefix the option with a `-D` (ex: `-DUSE_LIBSNDFILE=on`).

### Windows

To compile on Windows, you must include the dependencies in the dependencies' folder. Each of the dependencies must be in the appropriate folder.

- PortAudio -> portaudio
- FLAC -> flac
- libsndfile -> libsndfile

In each dependencies' folder, the includes files must be in the include directory and the libraries in the lib folder.

### Linux

Install the dependencies' library using your package manager, they are available is most distributions.

## API Documentation

To use the **SAL** library, include the `AudioPlayer.h` file. All the classes are available in the **SAL** namespace.

There are two needed class, the **AudioPlayer** class and the **CallbackInterface** class.
The **AudioPlayer** class is the main class, it is used to instantiate the SAL library and all the interactions (ex: open a file, play, pause, etc.) are made with this class.
The **CallbackInterface** is an interface between your callback functions and the SAL library. When a state of the library change, your callback functions are called.

### AudioPlayer class

- ``` C++
  static AudioPlayer* instance();
  ```
  - Create or return an instance of the AudioPlayer class. The instance is deleted by itself. If you need to delete it yourself, use either the `deinit` static function or the `delete` operator.

- ``` C++
  static void deinit();
  ```
  - Destroy the instance of the class.

- ``` C++
  void open(const std::string& filePath, bool clearQueue = false);
  ```
  - Add a file into the list queue, waiting to be played.
    - **filePath**: the path to the file to open.
    - **clearQueue**: Stop streaming and clear the current playing list. If the player was playing, it will start automatically playing the new file.

- ``` C++
  bool isReady(bool isWaiting = false);
  ```
  - Are audio files ready to play or are playing.
    - **isWaiting**: If true, the method hang until the next iteration of the main loop.

- ``` C++
  inline void play() noexcept;
  ```
  - Start playing or resume.

- ``` C++
  inline void pause() noexcept;
  ```
  - Pause the stream.

- ``` C++
  inline void stop() noexcept;
  ```
  - Stop the stream and clear the playing list.

- ``` C++
  bool isPlaying(bool isWaiting = false);
  ```
  - Return true if the player is playing.
    - **isWaiting**: If true, the method hang until the next iteration of the main loop.

- ``` C++
  inline void seek(size_t pos, bool inSeconds = true) noexcept;
  ```
  - Seek to a position in the currently streamed audio file.
    - **pos**: Position to seek.
    - **isSeconds**: If true, position is in seconds, otherwise, position is in frames.

- ``` C++
  inline void next() noexcept;
  ```
  - Play next audio file.

- ``` C++
  inline size_t streamSize(TimeType timeType = TimeType::SECONDS) const noexcept;
  ```
  - Return the stream size (size of the PCM data in the currently played audio file).
    - **timeType**: In which type the size should be return. Either `TimeType::SECONDS` or `TimeType::FRAMES`.

- ``` C++
  inline size_t streamPos(TimeType timeType = TimeType::SECONDS) const noexcept;
  ```
  - Return the stream position of the currently played audio file.
    - **timeType**: In which type the size should be return. Either `TimeType::SECONDS` or `TimeType::FRAMES`.

- ``` C++
  inline CallbackInterface& callback() noexcept;
  ```
  - Return the CallbackInterface instance created by AudioPlayer.

- ``` C++
  inline bool isInit() const;
  ```
  - Return true if the PortAudio API is initialized.

- ``` C++
  inline bool isRunning() const;
  ```
  - Return true if the PortAudio API is initialized and if the main loop is running.

- ``` C++
  inline void quit() noexcept;
  ```
  - Stop streaming and stop the AudioPlayer main loop. Cannot be restarted.

### CallbackInterface class

All the callback parameters are **std::function**.

- ``` C++
  void addStartFileCallback(StartFileCallback callback);
  ```
  - Callback called when a new file start to be streamed. Callback signature: `void(const std::string&)`.

- ``` C++
  void addEndFileCallback(EndFileCallback callback);
  ```
  - Callback called when a file stop to be streamed. Callback signature: `void(const std::string&)`.

- ``` C++
  void addStreamPosChangeCallback(StreamPosChangeCallback callback, TimeType timeType = TimeType::SECONDS);
  ```
  - Callback called when the stream position change. Callback signature: `void(size_t)`.

- ``` C++
  void addStreamPausedCallback(StreamPausedCallback callback);
  ```
  - Callback called when the stream is paused. Callback signature: `void()`.

- ``` C++
  void addStreamPlayingCallback(StreamPlayingCallback callback);
  ```
  - Callback called when the stream start playing or is resuming. Callback signature: `void()`.

- ``` C++
  void addStreamStoppingCallback(StreamStoppingCallback callback);
  ```
  - Callback called when the stream is stopping. Callback signature: `void()`.

- ``` C++
  void addStreamBufferingCallback(StreamBufferingCallback callback);
  ```
  - Callback called when the stream is buffering (not enough data to play). Callback signature: `void()`.

- ``` C++
  void addStreamEnoughBufferingCallback(StreamEnoughBufferingCallback callback);
  ```
  - Callback called when the stream has enough buffering. Callback signature: `void()`.

- ``` C++
  void addIsReadyChangedCallback(IsReadyChangedCallback callback);
  ```
  - Callback called when the isReady property is changing. Callback signature: `void(bool)`.

## License

The library is licensed under the **MIT** license. Check the [LICENSE](LICENSE) file.
