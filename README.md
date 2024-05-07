## Overview
This repository contains two prototypes developed within the current semester. 

WIP - readme to be updated


---

flaskServerWithEffects is depreciated
=======
## Prototype 1: Flask Server with Effects (Deprecated)
This prototype is a Flask application that renders three.JS code and runs a simulation of the board output. 


### How to Run
1. Download and launch YOLOv9 in a way that it saves all recognized objects in a text file.
2. Start `eventTrigArch2.py`. It will read the file and wait for events (e.g., cell phone or bottle detected).
3. Run `simulation.py` to have an LED PCB wall simulation. It will project 3D animation from JS to a 2D NumPy array.

### Features
- Launches a pixelated video output mimicking mirror mode on the wall.
- Displays video output effects when showing a cell phone or bottle to the camera (pixel size change and wave distortion).
- The cell phone event makes a floating cube leave traces on the screen, resembling drawing with it.
- Includes code for cube and sphere interaction. Collision detection makes the sphere turn red.
- A function to make the sphere appear when an event is recognized (requires a fix).
- Events send requests to the server, causing animation or video to change.

## Prototype 2: Flask 2D Server
This prototype generates a particle game with PyGame.

### How to Run
1. Launch `contourWallAI_edges_Array.py`. It will render the game window and launch the particle.

### Controls
- Press "Space" to launch a hand controller. It uses MediaPipe to capture a hand and lets the user control the particle.
- Press "b" to randomly place a square block on the screen. Multiple presses will create more blocks.
- Press "p" to emit more particles.

### Features
- The particle has acceleration and damping. When you virtually push it or hit it with a hand, it will move for some time and then stop.
- A particle and a block can collide. When the collision is detected, the block will be erased or consumed by the particle, mimicking brick pong or snake 2D games.
- More particles will float around, hit walls/edges of the screen, and eventually group in corners (under development).
- Particle behavior must be updated and connected to the hand controller, allowing users to influence the particle swarm.

## Future Work
This prototype must acquire more components with genAI text2picture or picture2picture models capable of generating pixel art, as well as LLM for conversational mode. Modes must switch when certain events are recognized, allowing various human-AI interaction scenarios.

## Additional Scripts
The repo has several other scripts for person and noise level recognition. These are required to create a simpler, less demanding version of the event-driven architecture to be deployed on Jetson Nano. There are also some minor experiments with Torch and NumPy arrays for image generation.

## Prototype 3: Ellie Connected
This directory contains connected components. 

