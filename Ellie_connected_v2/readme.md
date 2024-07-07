This folder contains Event-Driven Architecture prototypes and relevant components realised with Python. 

**modeSelection** - event-driven master script: animation, mode selection with number of fingers recognition, brick pong with mediapipe hand controller

**Demo:** https://drive.google.com/file/d/14VgP01R4UeSrjxccz6_b1pInGMkk3tBn/view?usp=sharing 

**brickPongVersions** - contains various attempts to optimise Python code with CuPy, Numba and other methods to make it run efficiently on Jetson (not possible to use with CUDA 10.02 which is max for Jetson Nano 2GB/4GB)

**versions** - partial implementations of event-driven architecture (e.g. animation to mode selection, counters, detectors, controllers, etc.)

**Flask server to handle events** - simple flask server implementation to receive requests and trigger events 

