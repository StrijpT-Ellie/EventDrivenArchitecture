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
#include <opencv2/opencv.hpp>  // Include OpenCV headers

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define PADDLE_WIDTH 100
#define PADDLE_HEIGHT 20
#define BALL_SIZE 10
#define FRAME_RATE 60  // Target frame rate
#define BRICK_ROWS 5
#define BRICK_COLS 10
#define BRICK_WIDTH (WINDOW_WIDTH / BRICK_COLS)
#define BRICK_HEIGHT 20

bool signal_received = false;

void sig_handler(int signo) {
    if (signo == SIGINT) {
        LogVerbose("received SIGINT\n");
        signal_received = true;
    }
}

int usage()
{
    printf("usage: combinedPaddle [--help] [--network=NETWORK] ...\n");
    printf("                input_URI [output_URI]\n\n");
    printf("Run pose estimation DNN on a video/image stream.\n");
    printf("See below for additional arguments that may not be shown above.\n\n");
    printf("positional arguments:\n");
    printf("    input_URI       resource URI of input stream  (see videoSource below)\n");
    printf("    output_URI      resource URI of output stream (see videoOutput below)\n\n");

    printf("%s", poseNet::Usage());
    printf("%s", videoSource::Usage());
    printf("%s", videoOutput::Usage());
    printf("%s", Log::Usage());

    return 0;
}

struct Ball {
    float x, y;
    float vx, vy;
};

struct Paddle {
    float x, y;
};

struct Brick {
    float x, y;
    bool active;
};

void initialize_bricks(std::vector<Brick>& bricks) {
    bricks.clear();
    for (int row = 0; row < BRICK_ROWS; ++row) {
        for (int col = 0; col < BRICK_COLS; ++col) {
            bricks.push_back(Brick{static_cast<float>(col * BRICK_WIDTH), static_cast<float>(row * BRICK_HEIGHT), true});
        }
    }
}

void update_paddle(Paddle &paddle, float hand_x) {
    // Flip hand_x by mirroring it around the center of the display width
    float inverted_hand_x = WINDOW_WIDTH - hand_x;
    paddle.x = inverted_hand_x - PADDLE_WIDTH / 2;

    // Clamp the paddle's position within the screen bounds
    if (paddle.x < 0) paddle.x = 0;
    if (paddle.x + PADDLE_WIDTH > WINDOW_WIDTH) paddle.x = WINDOW_WIDTH - PADDLE_WIDTH;
}

void update_ball(Ball &ball, const Paddle &paddle, std::vector<Brick>& bricks) {
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

    // Check for collisions with bricks
    bool all_bricks_destroyed = true;
    for (auto& brick : bricks) {
        if (brick.active) {
            all_bricks_destroyed = false;
            if (ball.y < brick.y + BRICK_HEIGHT && ball.y + BALL_SIZE > brick.y &&
                ball.x < brick.x + BRICK_WIDTH && ball.x + BALL_SIZE > brick.x) {
                brick.active = false;
                ball.vy = -ball.vy; // Bounce vertically
            }
        }
    }

    // Reset bricks if all are destroyed
    if (all_bricks_destroyed) {
        initialize_bricks(bricks);
    }

    // Check for bottom collision (game over)
    if (ball.y + BALL_SIZE > WINDOW_HEIGHT) {
        // Reset ball position and velocity
        ball.x = WINDOW_WIDTH / 2;
        ball.y = WINDOW_HEIGHT / 2;
        ball.vx = 4; // Increased speed
        ball.vy = 4; // Increased speed
    }
}

bool detect_hand(const std::vector<poseNet::ObjectPose>& poses, float &hand_x) {
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
            return true;
        }
    }
    return false;
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

    Ball ball = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 4, 4};  // Increased speed
    Paddle paddle = {WINDOW_WIDTH / 2 - PADDLE_WIDTH / 2, WINDOW_HEIGHT - PADDLE_HEIGHT};
    float hand_x = WINDOW_WIDTH / 2;
    std::vector<Brick> bricks;
    initialize_bricks(bricks);

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
        bool hand_detected = detect_hand(poses, hand_x);

        // Detect the two-finger gesture and update hand_x for paddle movement
        if (detect_two_fingers(poses, hand_x)) {
            // Debugging: Print the hand coordinates for the gesture
            std::cout << "Hand coordinates for gesture: (" << hand_x << ")" << std::endl;
        }

        // Update game objects
        if (hand_detected) {
            update_paddle(paddle, hand_x);
        }
        update_ball(ball, paddle, bricks);

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

        // Draw bricks
        for (const auto& brick : bricks) {
            if (brick.active) {
                SDL_Rect brick_rect = {(int)brick.x, (int)brick.y, BRICK_WIDTH, BRICK_HEIGHT};
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                SDL_RenderFillRect(renderer, &brick_rect);
            }
        }

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
