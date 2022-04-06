Camera on macOS
================

In order to easily simulate Home Accessory cameras it is possible to run the ADK on macOS and stream pre-recorded video
clips to a resident or controller. This step by step guide describes how to create the video clips and run ADK camera
applications and generate motion notifications for the cameras to residents.

``` Note::
    ADK camera application on macOS doesn't support using live camera at this time.
```

- [Step 1 - Setting up a Development Platform](#step-1-setting-up-a-development-platform)
- [Step 2 - Compiling the Camera Application](#step-2-compiling-the-camera-application)
- [Step 3 - Generating the Video Clips](#step-3-generating-the-video-clips)
- [Step 4 - Running the Camera Application](#step-4-running-the-camera-application)

### Step 1 - Setting up a Development Platform
Please follow instructions at the [Getting Started](getting_started.md) to setup the development platform.

### Step 2 - Compiling the Camera Application

```sh
make TARGET=Darwin APPS=VideoDoorbell
```

### Step 3 - Generating the Video Clips
ADK comes with some pre-built video clips that are used by Camera applications. If you are planning
on using the video clips that came with ADK then you can skip this step. However, if you want to use your own video
clips then you will need to format the clips into raw h264 frames. Please follow the instructions below:

#### Installing ffmpeg

```sh
brew install ffmpeg
```

#### Creating Raw H264 Clips

```sh
ffmpeg -I <video_clip_to_extract> -vcodec libx264 -bf 0 -b:v 800k -x264-params keyint=120 1920x1080.h264
ffmpeg -I <video_clip_to_extract> -vcodec libx264 -bf 0 -b:v 800k -x264-params keyint=120 1280x720.h264
ffmpeg -I <video_clip_to_extract> -vcodec libx264 -bf 0 -b:v 800k -x264-params keyint=120 640x480.h264
```

### Step 4 - Running the Camera Application

#### Using Pre-built Video Clips

```sh
./Output/Darwin-x86_64-apple-darwin$(uname -r)/Debug/IP/Applications/VideoDoorbell.OpenSSL
```

#### Using Custom Video Clips

```sh
./Output/Darwin-x86_64-apple-darwin$(uname -r)/Debug/IP/Applications/VideoDoorbell.OpenSSL -m <path_to_video_clip>
```

- `-m` is to specify the path to the custom video clip generated in *Step 3* above.

### Step 5 - Triggering Motion Events

HomeKit cameras can be set to allow recording of camera streams on detecting motion events. This can be enabled by
setting *Stream & Allow Recording* in *Recording Options* in *Home* application on an iOS device. The ADK camera
application can then be forced to mimic motion events to start a video recording session. To trigger a
motion event, do the following:

To trigger a motion event with a pre-built video clip that matches the configured camera resolution:

``` tabs::

    .. group-tab:: macOS

        - In Terminal window 1, launch the sample ADK application ``./Output/Darwin-x86_64-apple-darwin$(uname -r)/Debug/IP/Applications/VideoDoorbell.OpenSSL``
        - In Terminal window 2, ``echo triggerMotion > .command.input``

```

To trigger a motion event with a a custom video clip:

``` tabs::

    .. group-tab:: macOS

        - In Terminal window 1, launch the sample ADK application ``./Output/Darwin-x86_64-apple-darwin$(uname -r)/Debug/IP/Applications/VideoDoorbell.OpenSSL``
        - In Terminal window 2, ``echo "triggerMotion -m <video_clip_path>" > .command.input``

```

``` Note::
    Video recording session will automatically get terminated when the end of the video clip being used is reached.
```

For more information, please read [Buttons and Unix Signals](interact_with_applications.html#buttons-and-unix-signals).
