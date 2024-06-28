This folder containes code to be used with jetson-inference posenet. 

combined - game implementations that use posenet and directly pass detected hand keypoints coordinates to control moving bar or snake

listeners - these game implementations listen to named pipe for recognised hand coordinates which must come from one of posenet.cpp versions from poseNetVersions folder

poseNetVersions - posenet.cpp files updated to log coordinates and write to named pipe

---
---
How to use 

Use default Jetson Ubuntu 18.04 image 
Install openCV with CUDA enabled 

Clone jetson-inference repo

How to run hand recognition with posenet on jetson nano 

git clone https://github.com/dusty-nv/jetson-inference

git submodule update –init

cd ~/Desktop/jetson-inference/utils/python/bindings

nano PyCUDA.cpp

Add the following line at the top of the file to define the `PYLONG_FROM_PTR` macro:

#define PYLONG_FROM_PTR(ptr) PyLong_FromVoidPtr(ptr)

   Update the `cudaStreamWaitEvent` call to include the `unsigned int flags` parameter, typically set to 0:

   PYCUDA_ASSERT(cudaStreamWaitEvent(stream, event, 0));

Save and exit

cd jetson-inference

mkdir build

   cd ~/Desktop/jetson-inference/build
   make clean
   cmake ../
   make
   ```

allow permissions 
sudo chmod -R 755 ~/Desktop/jetson-inference/build


to run

cd jetson-inference/build/aarch64/bin

 
./posenet --network=resnet18-hand --topology=~/Desktop/jetson-inference/data/networks/Pose-ResNet18-Hand/hand_pose.json /dev/video0–



install dependencies
sudo apt-get install libgstreamer1.0-dev gstreamer1.0-tools gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly

—
Check cameras

gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,format=YUY2,width=640,height=480 ! videoconvert ! autovideosink

gst-launch-1.0 v4l2src device=/dev/video0 ! image/jpeg,width=640,height=480 ! jpegdec ! videoconvert ! autovideosink

./posenet --network=resnet18-hand --topology=~/Desktop/jetson-inference/data/networks/Pose-ResNet18-Hand/hand_pose.json --input-width=640 --input-height=480 --input-codec=mjpeg /dev/video0

—

To work with the copy of original posenet.cpp 
cd /jetson-inference/examples/posenet 

clone file
cp posenet.cpp my_posenet.cpp

Comment out GLOB, change cuda_add_executabl, add new lines inside the new file 

#file(GLOB posenetSources *.cpp)
#file(GLOB posenetIncludes *.h )

#cuda_add_executable(posenet ${posenetSources})
cuda_add_executable(posenet posenet.cpp)
target_link_libraries(posenet jetson-inference)
install(TARGETS posenet DESTINATION bin)

#New executable 
cuda_add_executable(my_posenet my_posenet.cpp)
target_link_libraries(my_posenet jetson-inference)
install(TARGETS my_posenet DESTINATION bin)

Use code from poseNetVersions which saves coordinates to named pipe
my_posenet_saveToNamedPipe.cpp

Run new file
./my_posenet --network=resnet18-hand --topology=~/Desktop/jetson-inference/data/networks/Pose-ResNet18-Hand/hand_pose.json --input-width=640 --input-height=480 --input-codec=mjpeg /dev/video0


Read named pipe 
tail -f /tmp/movement_pipe

Resources usage 
sudo tegrastats

System log
sudo dmesg

—-
Find files at 

jetson-inference/c
poseNet.h
poseNet.cpp

— — —
CmakeList for games

cmake_minimum_required(VERSION 3.10)
project(brickPong)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(brickPong brickPong.cpp)
target_link_libraries(brickPong ${OpenCV_LIBS})

—
cmake_minimum_required(VERSION 3.10)
project(brickPong)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Original executable
add_executable(brickPong brickPong.cpp)
target_link_libraries(brickPong ${OpenCV_LIBS})

# New executable for the updated version
add_executable(brickPong_Updated brickPong_Updated.cpp)
target_link_libraries(brickPong_Updated ${OpenCV_LIBS})

—

How to run games with hand controller 

Run my_posenet_2fingers first (it will launch and close due to failing to open a named pipe) 
This will create a pipe 

Then open the pipe with: 
tail -f /tmp/movement_pipe 

then run posenet 2 fingers again

then run brickPong 

as well brickpong and snake listen to the pipe (can launch them instead of pipe listening with tail) 

—
—
Compilation example 
 g++ -o paddle_ball paddle_ball.cpp -lSDL2

Use dbus-launch for different environment when running 
sudo dbus-launch ./paddle_ball 

—

cmake_minimum_required(VERSION 3.10)

project(poseNetBrickPong)

# Find OpenCV
find_package(OpenCV REQUIRED)

# Find SDL2
find_package(SDL2 REQUIRED)

# Include directories
include_directories(${OpenCV_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/../jetson-inference)

# Source files
set(SOURCES my_posenet.cpp)

# New executable
cuda_add_executable(poseNetBrickPong ${SOURCES})
target_link_libraries(poseNetBrickPong jetson-inference ${OpenCV_LIBS} ${SDL2_LIBRARIES})
install(TARGETS poseNetBrickPong DESTINATION bin)


—-
--- --- ---


