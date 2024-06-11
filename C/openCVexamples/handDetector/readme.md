This directory contains to versions of the simplest hand Detector with openCV. 
They are very slow, using some CUDA, but it is not enough. 
As well, accuaracy of detection is also very low. 

Reliable hand detectors are mostly with Python like mediaPipe. 
It is said that mediapipe offers C++ support. 
However, it requires bazel. 

Trying to install bazel previously destroyed CUDA support for openCV what led to system reinstallation. 
Thus, not going for mediapipe with C++ support in order to avoid conflicts. 


