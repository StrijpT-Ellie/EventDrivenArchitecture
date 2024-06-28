#include <SDL2/SDL.h>
#include "videoSource.h"
#include "videoOutput.h"
#include "poseNet.h"
#include <signal.h>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <opencv2/opencv.hpp>  // Include OpenCV headers

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define GRID_SIZE 20
#define FRAME_RATE 60  // Target frame rate

bool signal_received = false;

void sig_handler(int signo) {
    if (signo == SIGINT) {
        LogVerbose("received SIGINT\n");
        signal_received = true;
    }
}

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
    snake.body.clear();
    snake.body.push_back({WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2});
    snake.velocity = {0, -GRID_SIZE};  // Initial velocity upwards
    snake.radius = GRID_SIZE / 2;
    snake.segments_to_add = 0;
}

void initialize_food(std::vector<Particle> &food_particles, int quantity) {
    food_particles.clear();
    srand(time(0));
    for (int i = 0; i < quantity; ++i) {
        Particle food;
        food.x = rand() % (WINDOW_WIDTH / GRID_SIZE) * GRID_SIZE;
        food.y = rand() % (WINDOW_HEIGHT / GRID_SIZE) * GRID_SIZE;
        food.radius = GRID_SIZE / 2;
        food_particles.push_back(food);
    }
}

void update_snake(Snake &snake, float hand_x, float hand_y, bool hand_detected, std::vector<Particle> &food_particles) {
    if (hand_detected) {
        // Normalize and flip hand_x and hand_y to be within the display range
        float normalized_hand_x = WINDOW_WIDTH - (hand_x / 1280.0f) * WINDOW_WIDTH;  // Assuming the input resolution is 1280x720
        float normalized_hand_y = (hand_y / 720.0f) * WINDOW_HEIGHT;

        SDL_Point head_position = snake.body[0];
        SDL_Point hand_position = {static_cast<int>(normalized_hand_x), static_cast<int>(normalized_hand_y)};
        SDL_Point direction = {hand_position.x - head_position.x, hand_position.y - head_position.y};

        if (abs(direction.x) > abs(direction.y)) {
            // Move horizontally
            snake.velocity.x = (direction.x > 0) ? GRID_SIZE : -GRID_SIZE;
            snake.velocity.y = 0;
        } else {
            // Move vertically
            snake.velocity.x = 0;
            snake.velocity.y = (direction.y > 0) ? GRID_SIZE : -GRID_SIZE;
        }
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

bool detect_hand(const std::vector<poseNet::ObjectPose>& poses, float &hand_x, float &hand_y) {
    for (const auto& pose : poses) {
        int index_fingertip = pose.FindKeypoint(8);  // index_finger_4
        int middle_fingertip = pose.FindKeypoint(12);  // middle_finger_4
        int palm = pose.FindKeypoint(0);  // palm

        if (index_fingertip >= 0 && middle_fingertip >= 0 && palm >= 0) {
            const auto& kp_index = pose.Keypoints[index_fingertip];
            const auto& kp_middle = pose.Keypoints[middle_fingertip];
            const auto& kp_palm = pose.Keypoints[palm];

            // Log the coordinates if the keypoints are detected
            std::cout << "Index finger: (" << kp_index.x << ", " << kp_index.y << ")" << std::endl;
            std::cout << "Middle finger: (" << kp_middle.x << ", " << kp_middle.y << ")" << std::endl;

            hand_x = kp_index.x;  // Use the x-coordinate of the index fingertip for control
            hand_y = kp_index.y;  // Use the y-coordinate of the index fingertip for control
            return true;
        }
    }
    return false;
}

bool detect_two_fingers(const std::vector<poseNet::ObjectPose>& poses, float &hand_x, float &hand_y) {
    for (const auto& pose : poses) {
        int index_fingertip = pose.FindKeypoint(8);  // index_finger_4
        int middle_fingertip = pose.FindKeypoint(12);  // middle_finger_4
        int palm = pose.FindKeypoint(0);  // palm

        if (index_fingertip >= 0 && middle_fingertip >= 0 && palm >= 0) {
            const auto& kp_index = pose.Keypoints[index_fingertip];
            const auto& kp_middle = pose.Keypoints[middle_fingertip];
            const auto& kp_palm = pose.Keypoints[palm];

            if (kp_index.y < kp_palm.y && kp_middle.y < kp_palm.y) {
                float distance = sqrt(pow(kp_index.x - kp_middle.x, 2) + pow(kp_index.y - kp_middle.y, 2));
                if (distance > 30 && distance < 80) {  // Example thresholds, adjust as necessary
                    hand_x = kp_index.x;  // Use the x-coordinate of the index fingertip for control
                    hand_y = kp_index.y;  // Use the y-coordinate of the index fingertip for control
                    return true;
                }
            }
        }
    }
    return false;
}

int main(int argc, char** argv) {
    commandLine cmdLine(argc, argv);

    if (cmdLine.GetFlag("help"))
        return 0;

    if (signal(SIGINT, sig_handler) == SIG_ERR)
        LogError("can't catch SIGINT\n");

    videoSource* input = videoSource::Create(cmdLine, ARG_POSITION(0));
    if (!input) {
        LogError("my_posenet: failed to create input stream\n");
        return 1;
    }

    videoOutput* output = videoOutput::Create(cmdLine, ARG_POSITION(1));
    if (!output) {
        LogError("my_posenet: failed to create output stream\n");
        SAFE_DELETE(input);
        return 1;
    }

    poseNet* net = poseNet::Create(cmdLine);
    if (!net) {
        LogError("my_posenet: failed to initialize poseNet\n");
        SAFE_DELETE(input);
        SAFE_DELETE(output);
        return 1;
    }

    const uint32_t overlayFlags = poseNet::OverlayFlagsFromStr(cmdLine.GetString("overlay", "links,keypoints"));

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
        LogError("Error: Could not initialize SDL\n");
        SAFE_DELETE(input);
        SAFE_DELETE(output);
        SAFE_DELETE(net);
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Snake Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        LogError("Error: Could not create SDL window\n");
        SAFE_DELETE(input);
        SAFE_DELETE(output);
        SAFE_DELETE(net);
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        LogError("Error: Could not create SDL renderer\n");
        SDL_DestroyWindow(window);
        SAFE_DELETE(input);
        SAFE_DELETE(output);
        SAFE_DELETE(net);
        SDL_Quit();
        return -1;
    }

    Snake snake;
    initialize_snake(snake);

    std::vector<Particle> food_particles;
    initialize_food(food_particles, 10); // Set initial quantity of food particles to 10

    float hand_x = WINDOW_WIDTH / 2;  // Initialize hand_x at the center
    float hand_y = WINDOW_HEIGHT / 2;  // Initialize hand_y at the center
    bool hand_detected = false;  // Flag to indicate if a hand is detected

    bool quit = false;
    SDL_Event e;

    // Set a fixed time step for the game loop
    Uint32 frame_delay = 1000 / FRAME_RATE;

    while (!quit && !signal_received) {
        Uint32 frame_start = SDL_GetTicks();

        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        uchar3* image = NULL;
        int status = 0;

        if (!input->Capture(&image, &status)) {
            if (status == videoSource::TIMEOUT)
                continue;

            break; // EOS
        }

        int frameWidth = input->GetWidth();
        int frameHeight = input->GetHeight();

        std::vector<poseNet::ObjectPose> poses;
        if (!net->Process(image, frameWidth, frameHeight, poses, overlayFlags)) {
            LogError("my_posenet: failed to process frame\n");
            continue;
        }

        // Detect the hand and log the keypoints
        hand_detected = detect_hand(poses, hand_x, hand_y);

        // Detect the two-finger gesture and update hand_x and hand_y for snake movement
        if (detect_two_fingers(poses, hand_x, hand_y)) {
            // Debugging: Print the hand coordinates for the gesture
            std::cout << "Hand coordinates for gesture: (" << hand_x << ", " << hand_y << ")" << std::endl;
        }

        // Update the snake based on hand coordinates and check for food collisions
        update_snake(snake, hand_x, hand_y, hand_detected, food_particles);

        // Check if all food particles are eaten
        if (food_particles.empty()) {
            initialize_snake(snake);
            initialize_food(food_particles, 10); // Reset quantity of food particles to 10
        }

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
    SAFE_DELETE(input);
    SAFE_DELETE(output);
    SAFE_DELETE(net);
    SDL_Quit();

    return 0;
}
