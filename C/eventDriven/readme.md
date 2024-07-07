# Motion Detection Script Management

This repository contains a master script that manages and switches between two motion detection scripts (`arrayNoVideo` and `fadingPixels`) based on detected movement. If no movement is detected for a certain period, the master script will switch between the two scripts.

**Demo:** https://drive.google.com/file/d/1IVOBHTk2JU5LjdZWM55VMYE9VPgmm1oh/view?usp=sharing 


## Components

1. **Master Script (`masterScript_3modes.cpp`)**: Manages and switches between the two motion detection scripts based on movement detection.
2. **Motion Detection Scripts**:
   - **arrayNoVideo.cpp**
   - **fadingPixels.cpp**
   - **video2Array_noFlickering.cpp**
   - **rippleEffect.cpp**

## Requirements

- OpenCV 4.x
- g++
- CUDA (optional, if using GPU acceleration)
## Installation

### Install Dependencies

#### Ubuntu

```sh
sudo apt update
sudo apt install -y build-essential cmake pkg-config libopencv-dev
```

#### macOS (using Homebrew)

```sh
brew update
brew install opencv
```

### Compile the Scripts

1. **Rename the C files to C++ files**:

```sh
mv arrayNoVideo.c arrayNoVideo.cpp
mv fadingPixels.c fadingPixels.cpp
```

2. **Compile the scripts using g++**:

```sh
g++ -o arrayNoVideo arrayNoVideo.cpp `pkg-config --cflags --libs opencv4`
g++ -o fadingPixels fadingPixels.cpp `pkg-config --cflags --libs opencv4`
g++ -o masterScript masterScript.cpp 
g++ -o masterScript_3modes masterScript_3modes.cpp 
```

3. **Set executable permissions (if needed)**:

```sh
chmod +x arrayNoVideo
chmod +x fadingPixels
chmod +x masterScript
chmod +x masterScript_3modes
```

## Usage

1. **Run the master script**:

```sh
./masterScript_3modes ./optimisedRipplePixels ./arrayNoVideo ./fadingPixels ./video2Array_noFlickering
```

or ./gameSelector2

gameSelector2.cpp launches game selector which allows to select a game 
Move hands on the left or right side of the screen for 6 sec to select 
It counts intensive movement on one side of the screen

2. **Monitor the named pipe in real-time** (optional, for debugging):

```sh
tail -f /tmp/movement_pipe
```

## Script Details

### masterScript.cpp

The master script manages the two motion detection scripts. It launches `arrayNoVideo` first and switches to `fadingPixels` if no movement is detected for 15 seconds. It will switch back to `arrayNoVideo` after 15 seconds if `fadingPixels` detects no movement.

### arrayNoVideo.cpp and fadingPixels.cpp

These scripts perform motion detection using OpenCV. When movement is detected, they write a timestamp to a named pipe (`/tmp/movement_pipe`). The master script reads from this pipe to determine if movement has been detected.

### Adding Timestamps

Both `arrayNoVideo.cpp` and `fadingPixels.cpp` have been updated to write timestamps to the named pipe whenever movement is detected. This helps the master script accurately track the timing of the movement detections.

## Troubleshooting

- **If the scripts switch modes despite movement**:
  - Ensure the named pipe is correctly created and accessible.
  - Verify the motion detection scripts write to the pipe by checking the output with `tail -f /tmp/movement_pipe`.
  - Ensure the master script reads from the pipe correctly and resets the timer based on detected movement.

## License

This project is licensed under the MIT License.
