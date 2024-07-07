# posenet

## Overview
This folder contains code for using `jetson-inference`'s `posenet` on Jetson Nano. It includes game implementations that utilize `posenet` for hand recognition and control.


**Brick Pong Demo:** https://drive.google.com/file/d/15immDvVE9rzHjOSAga4jwyM3pBoCWAVq/view?usp=sharing 

**Snake Demo:** https://drive.google.com/file/d/13E9lRFCfXW6GLsjpQeqEjfcrZtrPX8RK/view?usp=sharing 

## Folder Structure
- **combined**: Games using `posenet` to control elements directly with detected hand keypoints.
- **listeners**: Games listening to a named pipe for hand coordinates provided by modified `posenet` versions.
- **poseNetVersions**: Modified `posenet.cpp` files to log coordinates and write them to a named pipe.

## Setup

### Prerequisites
1. **Jetson Nano with Ubuntu 18.04**.
2. **OpenCV with CUDA enabled**.
3. **jetson-inference repository**.

### Installation
1. Clone the repository:
   ```bash
   git clone https://github.com/dusty-nv/jetson-inference
   git submodule update --init
   ```
2. Modify `PyCUDA.cpp`:
   ```bash
   cd ~/jetson-inference/utils/python/bindings
   nano PyCUDA.cpp
   ```
   Add the following line at the top:
   ```cpp
   #define PYLONG_FROM_PTR(ptr) PyLong_FromVoidPtr(ptr)
   ```
   Update `cudaStreamWaitEvent`:
   ```cpp
   PYCUDA_ASSERT(cudaStreamWaitEvent(stream, event, 0));
   ```
3. Build the project:
   ```bash
   cd ~/jetson-inference
   mkdir build
   cd build
   make clean
   cmake ..
   make
   sudo chmod -R 755 ~/jetson-inference/build
   ```

### Run `posenet` for Hand Recognition
```bash
cd ~/jetson-inference/build/aarch64/bin
./posenet --network=resnet18-hand --topology=~/jetson-inference/data/networks/Pose-ResNet18-Hand/hand_pose.json /dev/video0
```

### Install Additional Dependencies
```bash
sudo apt-get install libgstreamer1.0-dev gstreamer1.0-tools gstreamer1.0-plugins-{base,good,bad,ugly}
```

### Check Camera Functionality
```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,format=YUY2,width=640,height=480 ! videoconvert ! autovideosink
gst-launch-1.0 v4l2src device=/dev/video0 ! image/jpeg,width=640,height=480 ! jpegdec ! videoconvert ! autovideosink
```

## Modified `posenet.cpp`
1. Navigate to:
   ```bash
   cd ~/jetson-inference/examples/posenet
   cp posenet.cpp my_posenet.cpp
   ```
2. Edit `CMakeLists.txt`:
   ```cmake
   #file(GLOB posenetSources *.cpp)
   #file(GLOB posenetIncludes *.h)
   #cuda_add_executable(posenet ${posenetSources})
   cuda_add_executable(posenet posenet.cpp)
   target_link_libraries(posenet jetson-inference)
   install(TARGETS posenet DESTINATION bin)

   # New executable
   cuda_add_executable(my_posenet my_posenet.cpp)
   target_link_libraries(my_posenet jetson-inference)
   install(TARGETS my_posenet DESTINATION bin)
   ```
3. Use `my_posenet_saveToNamedPipe.cpp` for saving coordinates to a named pipe:
   ```bash
   ./my_posenet --network=resnet18-hand --topology=~/jetson-inference/data/networks/Pose-ResNet18-Hand/hand_pose.json --input-width=640 --input-height=480 --input-codec=mjpeg /dev/video0
   tail -f /tmp/movement_pipe
   ```

## Additional Resources
- **System Monitoring**:
  ```bash
  sudo tegrastats
  sudo dmesg
  ```

- **Files Location**:
  - `jetson-inference/c/poseNet.h`
  - `jetson-inference/c/poseNet.cpp`

## CMakeLists for Games
### `brickPong`
```cmake
cmake_minimum_required(VERSION 3.10)
project(brickPong)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(brickPong brickPong.cpp)
target_link_libraries(brickPong ${OpenCV_LIBS})

# New executable for the updated version
add_executable(brickPong_Updated brickPong_Updated.cpp)
target_link_libraries(brickPong_Updated ${OpenCV_LIBS})
```

### `poseNetBrickPong`
```cmake
cmake_minimum_required(VERSION 3.10)
project(poseNetBrickPong)

find_package(OpenCV REQUIRED)
find_package(SDL2 REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/../jetson-inference)

set(SOURCES my_posenet.cpp)

cuda_add_executable(poseNetBrickPong ${SOURCES})
target_link_libraries(poseNetBrickPong jetson-inference ${OpenCV_LIBS} ${SDL2_LIBRARIES})
install(TARGETS poseNetBrickPong DESTINATION bin)
```

### Compilation Example
```bash
g++ -o paddle_ball paddle_ball.cpp -lSDL2
sudo dbus-launch ./paddle_ball
```

## Running Games with Hand Controller
1. Run `my_posenet_2fingers` to create a named pipe:
   ```bash
   ./my_posenet_2fingers
   tail -f /tmp/movement_pipe
   ```
2. Run the desired game (e.g., `brickPong`).
