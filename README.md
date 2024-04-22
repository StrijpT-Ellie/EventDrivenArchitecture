The repo has two prototypes developed within the scope of the current semester. 

flaskServerWithEffects is depreciated

It consists of a Flask application rendering three.JS code and running a simulation of the board output 

To run the app download yolov9 and launch it in a way that it will save all recognised objects in a text file.
After yolo is running start eventTrigArch2.py - it will be reading the file and waiting for events (e.g. cell phone or bottle detected).
It will launch a pixelated video output mimicing mirror mode on the wall. 

Run the simulation.py file to have an LED PCB wall simulation - it will project 3D animation from JS to the 2D numPy array. 

Finally, you'll see video output effects when showing cell phone or bottle to the camera (pixel size change and wave distortion). 
And the cell phone event will make floating cube leave traces on the screen which resembles drawing with it. 

As well, the simulation has the code for cude and sphere interaction. Collision detection makes the sphere turn red. 
And there is a function to make the sphere appear when event is recognised, but it requires a fix. 

Events send requests to the server which causes animation or video to change. 

---

Current prototype is in flask2DServer directory.

This prototype generates a particle game with pyGame. 

Simply launch contourWallAI_edges_Array.py 

It will render the game window and launch the particle. (Particle must float at start creating a resting animation. However it is WIP and now the acceleration is disabled what causes the particle to stay in the corner.)

Press "Space" button and it will launch a handController. 
It uses mediapipe to capture a hand and it will let the user control the particle. 

The particle has acceleration and damping. When you virtually push it or hit it with a hand it will move for some time and then stop. 
There is no gravity at the moment, so it will not fall. 

If you press "b" a square block will appear randomly on the screen. 
Multiple pressing will create more blocks. 

A particle and a block can collide. When the collision is detected the block will be erased or consumed by the particle mimicing brick pong or snake 2D games. 
The goal is to create a simple and intuitive game.

When pressing "p" more particles will be emitted. They should have collision with the main controlled particle but this is under development. 
Particles will float around, hit walls/edges of the screen and eventually group in corners. 

Particle behaviour must be updtaed and connected to handController allowing users to influence the particle swarm. 

---

This prototype must acquire more components with genAI text2picture or picture2picture models capable of generating pixel art. As well as LLM for conversational mode. 
Modes must be switching when certain events are recognised allowing various human-AI interaction scenarios. 

---

The repo has several other scripts for a person and noise level recognition. These are required to create a simpler, less demanding version of the event-driven architecture to be deployed on Jetson Nano. 

As well there are some minor experiments with torch and numPy arrays for image generation. 
