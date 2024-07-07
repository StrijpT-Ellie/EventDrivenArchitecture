# EventDrivenArchitecture

## Overview
This repository contains prototypes and components of an AI event-driven architecture for controlling 8-bit LED screen simulation. The project includes several modes such as animations, drawing, games, and interaction with large language models (LLMs). 

## Folder Structure

1. **C**: Various animations, movement detection, and pose/hand recognition on Jetson Nano. Includes snake and brick pong games implemented in C/C++.
   - [Event-Driven Architecture Demo](https://drive.google.com/file/d/1IVOBHTk2JU5LjdZWM55VMYE9VPgmm1oh/view?usp=sharing)
   - [Brick Pong Demo](https://drive.google.com/file/d/15immDvVE9rzHjOSAga4jwyM3pBoCWAVq/view?usp=sharing)
   - [Snake Demo](https://drive.google.com/file/d/13E9lRFCfXW6GLsjpQeqEjfcrZtrPX8RK/view?usp=sharing)

2. **Ellie_connected**: MobileNet for user recognition, Mediapipe for BrickPong hand controller, autolaunch when user is detected.
   - [Demo](https://drive.google.com/file/d/1wkesS2F_0U0m1K3RxaZood4mVLbymh6d/view?usp=sharing)

3. **Ellie_connected_v2**: More solid version with resting animation and movement detection with OpenCV, mode selection with hand gesture recognition and brick pong with hand controller, relaunch when no movement.
   - [Demo](https://drive.google.com/file/d/14VgP01R4UeSrjxccz6_b1pInGMkk3tBn/view?usp=sharing)

4. **flaskServerWith3DEffects**: YOLOv9 object detection integrated to create video effects and animations.
   - [Demo](https://vimeo.com/929837304/6112b0460f?share=copy)

5. **soundAndPersonRecognition**: Scripts for detecting sound levels and recognizing persons using a simple OpenCV model.

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
- **whisper**
- **Jetson Nano**

## License
This project is licensed under the MIT License.
