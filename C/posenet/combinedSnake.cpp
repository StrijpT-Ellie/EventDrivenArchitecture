#include <SDL2/SDL.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <cmath>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define GRID_SIZE 20
#define FRAME_RATE 60  // Target frame rate

struct Particle {
    float x, y;
    int radius;
};

struct Snake {
    std::vector<SDL_Point> body;
    SDL_Point velocity;
    int radius;
    int segments_to_add;
};

void initialize_snake(Snake &snake) {
    snake.body.push_back({WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2});
    snake.velocity = {0, -GRID_SIZE};  // Initial velocity upwards
    snake.radius = GRID_SIZE / 2;
    snake.segments_to_add = 0;
}

void initialize_food(std::vector<Particle> &food_particles) {
    srand(time(0));
    for (int i = 0; i < 10; ++i) {
        Particle food;
        food.x = rand() % (WINDOW_WIDTH / GRID_SIZE) * GRID_SIZE;
        food.y = rand() % (WINDOW_HEIGHT / GRID_SIZE) * GRID_SIZE;
        food.radius = GRID_SIZE / 2;
        food_particles.push_back(food);
    }
}

void update_snake(Snake &snake, float hand_x, float hand_y, bool hand_detected, std::vector<Particle> &food_particles) {
    if (hand_detected) {
        // Flip hand_x by mirroring it around the center of the display width
        float inverted_hand_x = WINDOW_WIDTH - hand_x;

        SDL_Point center = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
        SDL_Point hand_position = {static_cast<int>(inverted_hand_x), static_cast<int>(hand_y)};
        SDL_Point direction = {hand_position.x - center.x, hand_position.y - center.y};

        float angle = atan2(direction.y, direction.x);
        snake.velocity.x = cos(angle) * GRID_SIZE;
        snake.velocity.y = sin(angle) * GRID_SIZE;
    }

    // Update position
    SDL_Point new_head_position = {snake.body[0].x + snake.velocity.x, snake.body[0].y + snake.velocity.y};

    // Check for collisions with the edges of the display
    if (new_head_position.x < 0) new_head_position.x = WINDOW_WIDTH - GRID_SIZE;
    if (new_head_position.x >= WINDOW_WIDTH) new_head_position.x = 0;
    if (new_head_position.y < 0) new_head_position.y = WINDOW_HEIGHT - GRID_SIZE;
    if (new_head_position.y >= WINDOW_HEIGHT) new_head_position.y = 0;

    // Add new head position
    snake.body.insert(snake.body.begin(), new_head_position);

    // Check for food collisions
    for (auto it = food_particles.begin(); it != food_particles.end(); ) {
        if (sqrt(pow(snake.body[0].x - it->x, 2) + pow(snake.body[0].y - it->y, 2)) < (snake.radius + it->radius)) {
            // Eat the food particle
            it = food_particles.erase(it);
            // Add a segment to the snake
            snake.segments_to_add += 1;
        } else {
            ++it;
        }
    }

    // Remove the last segment if no new segments are being added
    if (snake.segments_to_add == 0) {
        snake.body.pop_back();
    } else {
        snake.segments_to_add--;
    }
}

void draw_snake(SDL_Renderer* renderer, const Snake &snake) {
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green color
    for (const auto &segment : snake.body) {
        SDL_Rect rect = { segment.x, segment.y, GRID_SIZE, GRID_SIZE };
        SDL_RenderFillRect(renderer, &rect);
    }
}

void draw_food(SDL_Renderer* renderer, const std::vector<Particle> &food_particles) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color
    for (const auto &food : food_particles) {
        SDL_Rect rect = { static_cast<int>(food.x), static_cast<int>(food.y), GRID_SIZE, GRID_SIZE };
        SDL_RenderFillRect(renderer, &rect);
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

    SDL_Window* window = SDL_CreateWindow("Snake Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
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

    Snake snake;
    initialize_snake(snake);

    std::vector<Particle> food_particles;
    initialize_food(food_particles);

    float hand_x = WINDOW_WIDTH / 2;  // Initialize hand_x at the center
    float hand_y = WINDOW_HEIGHT / 2;  // Initialize hand_y at the center
    bool hand_detected = false;  // Flag to indicate if a hand is detected

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
            std::istringstream iss(buffer);
            std::string line;
            while (std::getline(iss, line)) {
                // Parse the coordinates from the line
                if (sscanf(line.c_str(), "Pose %*d, Keypoint %*d: (%f, %f)", &hand_x, &hand_y) == 2) {
                    // Hand detected
                    hand_detected = true;
                    // Debugging: Print the hand coordinates
                    std::cout << "Hand coordinates: (" << hand_x << ", " << hand_y << ")" << std::endl;
                }
            }
        } else {
            // No hand detected
            hand_detected = false;
        }

        // Update the snake based on hand coordinates and check for food collisions
        update_snake(snake, hand_x, hand_y, hand_detected, food_particles);

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw the snake
        draw_snake(renderer, snake);

        // Draw the food particles
        draw_food(renderer, food_particles);

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
    close(pipe_fd);

    return 0;
}
