#include "videoSource.h"
#include "videoOutput.h"
#include "poseNet.h"
#include <SDL2/SDL.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <cmath>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define PADDLE_WIDTH 100
#define PADDLE_HEIGHT 20
#define BALL_SIZE 10
#define FRAME_RATE 60  // Target frame rate

bool signal_received = false;

void sig_handler(int signo) {
    if (signo == SIGINT) {
        LogVerbose("received SIGINT\n");
        signal_received = true;
    }
}

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

bool detect_two_fingers(const std::vector<poseNet::ObjectPose>& poses, float &hand_x) {
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
        return usage();

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

    SDL_Window* window = SDL_CreateWindow("Paddle Ball", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
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

    Ball ball = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 2, 2};
    Paddle paddle = {WINDOW_WIDTH / 2 - PADDLE_WIDTH / 2, WINDOW_HEIGHT - PADDLE_HEIGHT};
    float hand_x = WINDOW_WIDTH / 2;

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

        std::vector<poseNet::ObjectPose> poses;
        if (!net->Process(image, input->GetWidth(), input->GetHeight(), poses, overlayFlags)) {
            LogError("my_posenet: failed to process frame\n");
            continue;
        }

        // Detect the gesture and update hand_x
        if (detect_two_fingers(poses, hand_x)) {
            // Debugging: Print the hand coordinates
            std::cout << "Hand coordinates: (" << hand_x << ")" << std::endl;
        }

        // Update game objects
        update_paddle(paddle, hand_x);
        update_ball(ball, paddle);

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw paddle
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect paddle_rect = {(int)paddle.x, (int)paddle.y, PADDLE_WIDTH, PADDLE_HEIGHT};
        SDL_RenderFillRect(renderer, &paddle_rect);

        // Draw ball
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_Rect ball_rect = {(int)ball.x, (int)ball.y, BALL_SIZE, BALL_SIZE};
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
    SAFE_DELETE(input);
    SAFE_DELETE(output);
    SAFE_DELETE(net);
    SDL_Quit();

    return 0;