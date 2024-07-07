This folder contains plain C examples. Tested on Mac M1.

sdl_example - hello world terminal 
sdl_letters - hello world rendered 
sdl_ttf_color_example - rendered letters change color 
sdl_ttf_bounce_example - letters move on the screen and bounce from edges 
sdl_ttf_control_example - move the letters on the screen using arrows (keyboard input) 

--
Compilation example 

//To compile 

// gcc -I/opt/homebrew/Cellar/sdl2/2.30.3/include/SDL2 -I/opt/homebrew/Cellar/sdl2_ttf/2.22.0/include/SDL2 -L/opt/homebrew/Cellar/sdl2/2.30.3/lib -L/opt/homebrew/Cellar/sdl2_ttf/2.22.0/lib -o sdl_ttf_bounce_example movingLetters.c -lSDL2 -lSDL2_ttf
 