#include <SDL2/SDL.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <iostream>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define PADDLE_WIDTH 100
#define PADDLE_HEIGHT 20
#define BALL_SIZE 10
#define FRAME_RATE 60  // Target frame rate

struct Ball {
    float x, y;
    float vx, vy;
};

struct Paddle {
    float x, y;
};

void update_paddle(Paddle &paddle, float hand_x) {
    // Flip hand_x by mirroring it around the center of the display width
    float inverted_hand_x = WINDOW_WIDTH - hand_x;
    paddle.x = inverted_hand_x - PADDLE_WIDTH / 2;

    // Clamp the paddle's position within the screen bounds
    if (paddle.x < 0) paddle.x = 0;
    if (paddle.x + PADDLE_WIDTH > WINDOW_WIDTH) paddle.x = WINDOW_WIDTH - PADDLE_WIDTH;
}

void update_ball(Ball &ball, const Paddle &paddle) {
    ball.x += ball.vx;
    ball.y += ball.vy;

    // Check for collisions with the edges of the display
    if (ball.x < 0 || ball.x + BALL_SIZE > WINDOW_WIDTH) {
        ball.vx = -ball.vx; // Bounce horizontally
    }
    if (ball.y < 0) {
        ball.vy = -ball.vy; // Bounce vertically
    }

    // Check for collisions with the paddle
    if (ball.y + BALL_SIZE > paddle.y && ball.x + BALL_SIZE > paddle.x && ball.x < paddle.x + PADDLE_WIDTH) {
        ball.vy = -ball.vy; // Bounce up
        ball.y = paddle.y - BALL_SIZE; // Move ball above the paddle
    }

    // Check for bottom collision (game over)
    if (ball.y + BALL_SIZE > WINDOW_HEIGHT) {
        // Reset ball position and velocity
        ball.x = WINDOW_WIDTH / 2;
        ball.y = WINDOW_HEIGHT / 2;
        ball.vx = 2;
        ball.vy = 2;
    }
}

int main(int argc, char** argv) {
    // Open the named pipe for reading
    const char* pipePath = "/tmp/movement_pipe";
    int pipe_fd = open(pipePath, O_RDONLY | O_NONBLOCK); // Open pipe in non-blocking mode
    if (pipe_fd == -1) {
        std::cerr << "Error: Could not open named pipe" << std::endl;
        return -1;
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
        std::cerr << "Error: Could not initialize SDL" << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Paddle Ball", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Error: Could not create SDL window" << std::endl;
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Error: Could not create SDL renderer" << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    Ball ball = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 2, 2};
    Paddle paddle = {WINDOW_WIDTH / 2 - PADDLE_WIDTH / 2, WINDOW_HEIGHT - PADDLE_HEIGHT};
    float hand_x = WINDOW_WIDTH / 2;

    bool quit = false;
    SDL_Event e;

    // Set a fixed time step for the game loop
    Uint32 frame_delay = 1000 / FRAME_RATE;

    while (!quit) {
        Uint32 frame_start = SDL_GetTicks();

        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        // Read from the named pipe
        char buffer[256];
        ssize_t bytesRead = read(pipe_fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            float hand_y;
            std::istringstream iss(buffer);
            std::string line;
            while (std::getline(iss, line)) {
                // Parse the coordinates from the line
                if (sscanf(line.c_str(), "Pose %*d, Keypoint %*d: (%f, %f)", &hand_x, &hand_y) == 2) {
                    // Debugging: Print the hand coordinates
                    std::cout << "Hand coordinates: (" << hand_x << ", " << hand_y << ")" << std::endl;
                }
            }
        }

        // Update game objects
        update_paddle(paddle, hand_x);
        update_ball(ball, paddle);

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw paddle
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect paddle_rect = { (int)paddle.x, (int)paddle.y, PADDLE_WIDTH, PADDLE_HEIGHT };
        SDL_RenderFillRect(renderer, &paddle_rect);

        // Draw ball
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_Rect ball_rect = { (int)ball.x, (int)ball.y, BALL_SIZE, BALL_SIZE };
        SDL_RenderFillRect(renderer, &ball_rect);

        // Update screen
        SDL_RenderPresent(renderer);

        // Frame delay
        Uint32 frame_time = SDL_GetTicks() - frame_start;
        if (frame_delay > frame_time) {
            SDL_Delay(frame_delay - frame_time);
        }
    }

    // Clean up
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    // Close the named pipe
    close(pipe_fd);

    return 0;
}
