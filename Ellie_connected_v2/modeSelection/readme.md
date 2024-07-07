This folder contains final Python results with implementation of event-driven architecture for Ellie ContourWall project. 

Working prototype tested on Windows 
modeSelect_handcontrol.py

1. Launches resting animation responsive to movement 
2. When a person is moving for 30 sec it shuts down the animation and launches a mode selection window 
3. It is possible to select a mode by showing a corresponding number of fingers 
4. When fingers are shown for 6 seconds the selected mode is highlighted green
5. Currently only a the game mode (brick pong) is supported  
6. Mode selection shuts down and launches the game 
7. It is possible to play the game with a hand controller and move the bar 
8. The game logs movement into the .txt file 
9. When now movement occurs for 30-60 sec it shuts down the game 
10. The resting animation is restarted and waits for the user to appear

--
Several versions are created to deploy on Jetson Nano 
Original version uses threding and it didn't work as expected on Jetson 
optimised versions use Multiprocessing 

eventDrivenForJetson.py (subprocess and threading)
eventDrivenMultiprocessing.py 
eventDrivenMultiprocessing_v2.py 

The script launches a game
- brickPong.py on Windows 
- brickpongForJetson.py on Jetson Nano
- brickpongForJetson_v2.py 