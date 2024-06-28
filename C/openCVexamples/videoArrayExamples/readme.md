This folder contains various C implementations of camera input 2 pixel array projection. 
Built to run on Jetson Nano with CUDA.

Open CV with CUDA enabled is required and can be installed with this tutorial: https://qengineering.eu/install-opencv-on-jetson-nano.html

g++ (rename files to .cpp (mv video2Array_LEDs.c video2ArrayLEDs.cpp))

sudo apt update
sudo apt install libopencv-dev

Compile 
g++ -o led_pcb_wall led_pcb_wall.cpp -I/usr/include/opencv4 -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio -lopencv_cudaimgproc -lopencv_cudawarping

---

#!/bin/bash

# Define the source file and output executable
SOURCE_FILE="led_pcb_wall.cpp"
OUTPUT_FILE="led_pcb_wall"

# Compile the program with g++
g++ -o $OUTPUT_FILE $SOURCE_FILE -I/usr/include/opencv4 -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio -lopencv_cudaimgproc -lopencv_cudawarping

# Check if the compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful. You can run the program using ./$OUTPUT_FILE"
else
    echo "Compilation failed. Please check for errors in the source code."
fi


---

Make file example 

# Compiler
CXX = gcc

# Compiler flags
CXXFLAGS = -I/usr/include/opencv4

# Linker flags
LDFLAGS = -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio -lopencv_cudaimgproc -lopencv_cudawarping

# Target executable
TARGET = led_pcb_wall

# Source files
SRCS = led_pcb_wall.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default rule
all: $(TARGET)

# Link the target executable
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile source files into object files
%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule to remove generated files
clean:
	rm -f $(OBJS) $(TARGET)
