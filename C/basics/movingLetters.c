//This script genertes moving letters that bounce from edges
//To compile 
// gcc -I/opt/homebrew/Cellar/sdl2/2.30.3/include/SDL2 -I/opt/homebrew/Cellar/sdl2_ttf/2.22.0/include/SDL2 -L/opt/homebrew/Cellar/sdl2/2.30.3/lib -L/opt/homebrew/Cellar/sdl2_ttf/2.22.0/lib -o sdl_ttf_bounce_example movingLetters.c -lSDL2 -lSDL2_ttf


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
    color.r = rand() % 256;
    color.g = rand() % 256;
    color.b = rand() % 256;
    color.a = 255;
    return color;
}

// Function to generate a random direction (-1 or 1)
int getRandomDirection() {
    return (rand() % 2 == 0) ? 1 : -1;
}

int main(int argc, char* argv[]) {
    // Initialize random number generator
    srand(time(0));

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        printf("TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    // Create a window
    SDL_Window *win = SDL_CreateWindow("Hello SDL_ttf", 100, 100, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (win == NULL) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Create a renderer
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == NULL) {
        SDL_DestroyWindow(win);
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Load a font
    TTF_Font *font = TTF_OpenFont("/Library/Fonts/Arial Unicode.ttf", 24);
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

    // Initial direction of the text movement
    int dirX = getRandomDirection();
    int dirY = getRandomDirection();

    // Main loop flag
    int quit = 0;

    // Event handler
    SDL_Event e;

    // While application is running
    while (!quit) {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            // User requests quit
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
        }

        // Move the text
        posX += dirX * 5;
        posY += dirY * 5;

        // Bounce the text off the edges
        if (posX <= 0 || posX >= SCREEN_WIDTH - 100) {
            dirX = -dirX;
            posX += dirX * 5; // Adjust position after bounce
        }
        if (posY <= 0 || posY >= SCREEN_HEIGHT - 50) {
            dirY = -dirY;
            posY += dirY * 5; // Adjust position after bounce
        }

        // Generate a random color for the text
        SDL_Color textColor = getRandomColor();

        // Create a surface with the text
        SDL_Surface *textSurface = TTF_RenderText_Solid(font, "Hello, SDL_ttf!", textColor);
        if (textSurface == NULL) {
            printf("TTF_RenderText_Solid Error: %s\n", TTF_GetError());
            TTF_CloseFont(font);
            SDL_DestroyRenderer(ren);
            SDL_DestroyWindow(win);
            TTF_Quit();
            SDL_Quit();
            return 1;
        }

        // Create a texture from the surface
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(ren, textSurface);
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

        // Get the dimensions of the text
        int textWidth = textSurface->w;
        int textHeight = textSurface->h;

        // Free the surface since it's no longer needed
        SDL_FreeSurface(textSurface);

        // Clear screen
        SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF); // White
        SDL_RenderClear(ren);

        // Render text
        SDL_Rect renderQuad = { posX, posY, textWidth, textHeight };
        SDL_RenderCopy(ren, textTexture, NULL, &renderQuad);

        // Update screen
        SDL_RenderPresent(ren);

        // Free the texture
        SDL_DestroyTexture(textTexture);

        // Delay for a short period to create the effect
        SDL_Delay(50);
    }

    // Free resources and close SDL
    TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
