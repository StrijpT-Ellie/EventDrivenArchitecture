## Overview

This project implements a real-time object detection system in a video stream from default camera using the MobileNetSSD (MobileNet Single Shot Multibox Detector) model. Another file is listening to person detector output in .txt and launches a game when a person is detected.

**Demo:** https://drive.google.com/file/d/1wkesS2F_0U0m1K3RxaZood4mVLbymh6d/view?usp=sharing 

-----

**real_time_object_detection.py** - test file for object detection with default webcam and MobileNet

Launch with:
python real_time_object_detection.py --prototxt MobileNetSSD_deploy.prototxt.txt --model MobileNetSSD_deploy.caffemodel

**real_time_object_detection_autoquit.py** - this file automatically closes when a person is detected
Recognised objects output saved to labels.txt

Launch with:
py -3.11 real_time_object_detection_autoquit.py --prototxt .\model\MobileNetSSD_deploy.prototxt.txt --model .\model\MobileNetSSD_deploy.caffemodel


**contourWallAI_blocksNRows_eventDriven.py** - listens to .txt file and launches brick pong game with mediapipe-based hand controller

-----

To run:

1. Close your camera with a paper or a hand 
2. Launch real_time_object_detection_autoquit.py 
3. Launch contourWallAI_blocksNRows_eventDriven.py
4. Open the camera 
5. real_time_object_detection_autoquit.py will detect a person and quit 
6. contourWallAI_blocksNRows_eventDriven.py will detect a person in .txt and launch a brick pong game

