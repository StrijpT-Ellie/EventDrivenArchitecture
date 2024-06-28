This folder contains implemetations of original posenet.cpp file with logging of detected hand/pose keypoints coordinates.

logCoordinates - logs in terminal 
saveToNamedPipe - saves to /tmp/movement_pipe
2fingers - logs coordinates only when two fingers are up instead of full hand 

---

These files must be placed in jetson-inference/examples/posenet.

CmakeLists file must be updated and use them instead of original posenet in compilation. 

