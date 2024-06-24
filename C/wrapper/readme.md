This directory contains an attempt to implement C++ wrapper. 
It wasn't tested. 

---


Make sure that the compiled Rust shared library (libcontourwall.so, libcontourwall.dll, or libcontourwall.dylib) is available in your project directory or a directory included in the library path.

contourwall_cpp/
├── CMakeLists.txt
├── contourwall.hpp
└── main.cpp

cmake_minimum_required(VERSION 3.10)
project(ContourWallCPP)

set(CMAKE_CXX_STANDARD 14)

# Set the path to the Rust shared library
set(RUST_LIBRARY_PATH "/path/to/rust/library")

# Add the include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Add the executable
add_executable(ContourWallCPP main.cpp)

# Link the Rust shared library
target_link_libraries(ContourWallCPP ${RUST_LIBRARY_PATH}/libcontourwall.so)

Replace /path/to/rust/library with the actual path to the directory containing the Rust shared library.

---

mkdir build
cd build
cmake ..
make
./ContourWallCPP
