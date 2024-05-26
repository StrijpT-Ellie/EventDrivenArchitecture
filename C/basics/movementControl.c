//This script allows to move a text in the screen using the arrow keys
//To compile 
//gcc -I/opt/homebrew/Cellar/sdl2/2.30.3/include/SDL2 -I/opt/homebrew/Cellar/sdl2_ttf/2.22.0/include/SDL2 -L/opt/homebrew/Cellar/sdl2/2.30.3/lib -L/opt/homebrew/Cellar/sdl2_ttf/2.22.0/lib -o sdl_ttf_control_example movementControl.c -lSDL2 -lSDL2_ttf


#include </opt/homebrew/Cellar/sdl2/2.30.3/include/SDL2/SDL.h>
#include </opt/homebrew/Cellar/sdl2_ttf/2.22.0/include/SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

// Function to generate a random color
SDL_Color getRandomColor() {
    SDL_Color color;
    color.r = rand() % 256;  // Random value for red component
    color.g = rand() % 256;  // Random value for green component
    color.b = rand() % 256;  // Random value for blue component
    color.a = 255;  // Alpha component set to opaque
    return color;
}

int main(int argc, char* argv[]) {
    srand(time(0));  // Seed the random number generator with the current time

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {  // Initialize SDL2
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1) {  // Initialize SDL_ttf
        printf("TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("Hello SDL_ttf", 100, 100, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);  // Create a window
    if (win == NULL) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);  // Create a renderer
    if (ren == NULL) {
        SDL_DestroyWindow(win);
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("/Library/Fonts/Arial Unicode.ttf", 24);  // Load a font
    if (font == NULL) {
        printf("TTF_OpenFont Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Initial position of the text
    int posX = (SCREEN_WIDTH - 100) / 2;
    int posY = (SCREEN_HEIGHT - 50) / 2;

    int quit = 0;  // Main loop flag
    SDL_Event e;  // Event handler

    while (!quit) {  // Main loop
        while (SDL_PollEvent(&e) != 0) {  // Handle events
            if (e.type == SDL_QUIT) {  // User requests quit
                quit = 1;
            } else if (e.type == SDL_KEYDOWN) {  // User presses a key
                switch (e.key.keysym.sym) {
                    case SDLK_UP:    // Move up
                        posY -= 10;
                        if (posY < 0) posY = 0;
                        break;
                    case SDLK_DOWN:  // Move down
                        posY += 10;
                        if (posY > SCREEN_HEIGHT - 50) posY = SCREEN_HEIGHT - 50;
                        break;
                    case SDLK_LEFT:  // Move left
                        posX -= 10;
                        if (posX < 0) posX = 0;
                        break;
                    case SDLK_RIGHT: // Move right
                        posX += 10;
                        if (posX > SCREEN_WIDTH - 100) posX = SCREEN_WIDTH - 100;
                        break;
                }
            }
        }

        SDL_Color textColor = getRandomColor();  // Generate a random color

        SDL_Surface *textSurface = TTF_RenderText_Solid(font, "Hello, SDL_ttf!", textColor);  // Create surface with text
        if (textSurface == NULL) {
            printf("TTF_RenderText_Solid Error: %s\n", TTF_GetError());
            TTF_CloseFont(font);
            SDL_DestroyRenderer(ren);
            SDL_DestroyWindow(win);
            TTF_Quit();
            SDL_Quit();
            return 1;
        }

        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(ren, textSurface);  // Create texture from surface
        if (textTexture == NULL) {
            printf("SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
            SDL_FreeSurface(textSurface);
            TTF_CloseFont(font);
            SDL_DestroyRenderer(ren);
            SDL_DestroyWindow(win);
            TTF_Quit();
            SDL_Quit();
            return 1;
        }

        int textWidth = textSurface->w;  // Get text width
        int textHeight = textSurface->h;  // Get text height
        SDL_FreeSurface(textSurface);  // Free the surface

        SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF);  // Set draw color to white
        SDL_RenderClear(ren);  // Clear the screen

        SDL_Rect renderQuad = { posX, posY, textWidth, textHeight };  // Set rendering rectangle
        SDL_RenderCopy(ren, textTexture, NULL, &renderQuad);  // Render text texture to screen

        SDL_RenderPresent(ren);  // Update screen
        SDL_DestroyTexture(textTexture);  // Destroy the texture

        SDL_Delay(50);  // Delay to create the effect
    }

    TTF_CloseFont(font);  // Free font
    SDL_DestroyRenderer(ren);  // Destroy renderer
    SDL_DestroyWindow(win);  // Destroy window
    TTF_Quit();  // Quit SDL_ttf
    SDL_Quit();  // Quit SDL

    return 0;
}
