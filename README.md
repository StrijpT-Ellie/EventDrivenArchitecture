# EventDrivenArchitecture

## Overview
This repository contains prototypes and components of an AI event-driven architecture for controlling 8-bit LED screen. The project includes several modes such as animations, drawing, games, and interaction with large language models (LLMs). 

## Folder Structure

- **C**: Various animations, movement detection, and pose/hand recognition on Jetson Nano. Includes snake and brick pong games implemented in C/C++.
- **Ellie_connected**: MobileNet for user recognition, Mediapipe for BrickPong hand controller, autolaunch when user is detected.
- **Ellie_connected_v2**: More solid version with resting animation and movement detection with openCV, mode selection with hand gesture recognition and brick pong with hand controller, aut relaunch when no movement.
- **flaskServerWith3DEffects**: YOLOv9 object detection integrated to create video effects and animations.
- **ollamaToPixelatedLetters**: Python scripts for sending requests to the Ollama server and projecting responses in a pixelated font.
- **soundAndPersonRecognition**: Scripts for detecting sound levels and recognizing persons using a simple OpenCV model.

## Technologies Used
- **Python**
- **C/C++**
- **YOLOv9**
- **Mediapipe**
- **MobileNet**
- **OpenCV**
- **PyGame**
- **NumPy**
- **Matplotlib**
- **Jetson Nano**

## License
This project is licensed under the MIT License.
