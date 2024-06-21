#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <fcntl.h>
#include <unistd.h>

#define DISPLAY_WIDTH 640
#define DISPLAY_HEIGHT 480
#define BAR_HEIGHT 20
#define BRICK_ROWS 5
#define BRICK_COLS 10
#define BRICK_WIDTH (DISPLAY_WIDTH / BRICK_COLS)
#define BRICK_HEIGHT 20

using namespace cv;
using namespace std;

struct FloatingBall {
    Point2f position;
    Point2f velocity;
    int radius;
    Scalar color;
};

struct Bar {
    Point2f position;
    int width;
    int height;
    Scalar color;
};

struct Brick {
    Point2f position;
    int width;
    int height;
    Scalar color;
    bool active;
};

void initialize_led_wall(Mat &led_wall) {
    led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);
}

void initialize_floating_ball(FloatingBall &ball) {
    ball.position = Point2f(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
    ball.velocity = Point2f(5, 5);  // Adjusted velocity for better control
    ball.radius = 20;
    ball.color = Scalar(0, 0, 255);  // Red color
}

void initialize_bar(Bar &bar) {
    bar.position = Point2f(0, DISPLAY_HEIGHT - BAR_HEIGHT);
    bar.width = 100;
    bar.height = BAR_HEIGHT;
    bar.color = Scalar(255, 0, 0);  // Blue color
}

void initialize_bricks(vector<Brick> &bricks) {
    bricks.clear();
    for (int i = 0; i < BRICK_ROWS; i++) {
        for (int j = 0; j < BRICK_COLS; j++) {
            Brick brick;
            brick.position = Point2f(j * BRICK_WIDTH, i * BRICK_HEIGHT);
            brick.width = BRICK_WIDTH;
            brick.height = BRICK_HEIGHT;
            brick.color = Scalar(0, 255, 0);  // Green color
            brick.active = true;
            bricks.push_back(brick);
        }
    }
}

void update_floating_ball(FloatingBall &ball, const Bar &bar, vector<Brick> &bricks) {
    ball.position += ball.velocity;

    // Check for collisions with the edges of the display
    if (ball.position.x - ball.radius < 0) {
        ball.velocity.x = abs(ball.velocity.x); // Bounce right
    } else if (ball.position.x + ball.radius > DISPLAY_WIDTH) {
        ball.velocity.x = -abs(ball.velocity.x); // Bounce left
    }
    if (ball.position.y - ball.radius < 0) {
        ball.velocity.y = abs(ball.velocity.y); // Bounce down
    } else if (ball.position.y + ball.radius > DISPLAY_HEIGHT) {
        ball.velocity.y = -abs(ball.velocity.y); // Bounce up
    }

    // Check for collisions with the bar
    Rect bar_rect(bar.position, Size(bar.width, bar.height));
    if (ball.position.x + ball.radius > bar_rect.x &&
        ball.position.x - ball.radius < bar_rect.x + bar_rect.width &&
        ball.position.y + ball.radius > bar_rect.y &&
        ball.position.y - ball.radius < bar_rect.y + bar_rect.height) {
        if (ball.position.y + ball.radius >= bar_rect.y && ball.position.y - ball.radius < bar_rect.y) {
            ball.velocity.y = -abs(ball.velocity.y); // Bounce up
        }
    }

    // Check for collisions with bricks
    for (auto &brick : bricks) {
        if (brick.active) {
            Rect brick_rect(brick.position, Size(brick.width, brick.height));
            if (ball.position.x + ball.radius > brick_rect.x &&
                ball.position.x - ball.radius < brick_rect.x + brick_rect.width &&
                ball.position.y + ball.radius > brick_rect.y &&
                ball.position.y - ball.radius < brick_rect.y + brick_rect.height) {
                brick.active = false;
                if (abs(ball.position.x - (brick_rect.x + brick_rect.width / 2)) > abs(ball.position.y - (brick_rect.y + brick_rect.height / 2))) {
                    ball.velocity.x = -ball.velocity.x;
                } else {
                    ball.velocity.y = -ball.velocity.y;
                }
            }
        }
    }
}

void update_bar(Bar &bar, float hand_x_position) {
    bar.position.x = hand_x_position - bar.width / 2;

    // Clamp the bar's position within the screen bounds
    if (bar.position.x < 0) bar.position.x = 0;
    if (bar.position.x + bar.width > DISPLAY_WIDTH) bar.position.x = DISPLAY_WIDTH - bar.width;
}

void draw_led_wall(Mat &led_wall, const FloatingBall &ball, const Bar &bar, const vector<Brick> &bricks) {
    // Clear the LED wall
    led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);

    // Draw the floating ball
    cv::circle(led_wall, ball.position, ball.radius, ball.color, FILLED);

    // Draw the bar
    rectangle(led_wall, Rect(bar.position.x, bar.position.y, bar.width, bar.height), bar.color, FILLED);

    // Draw the bricks
    for (const auto &brick : bricks) {
        if (brick.active) {
            rectangle(led_wall, Rect(brick.position.x, brick.position.y, brick.width, brick.height), brick.color, FILLED);
        }
    }
}

int main(int argc, char** argv) {
    // Open the default camera
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        printf("Error: Could not open camera\n");
        return -1;
    }

    // Set frame rate to reduce processing load
    cap.set(CAP_PROP_FPS, 15);

    // Create a window and resize it
    namedWindow("LED PCB Wall Simulation", WINDOW_NORMAL);
    resizeWindow("LED PCB Wall Simulation", DISPLAY_WIDTH, DISPLAY_HEIGHT);

    Mat frame, prev_frame;
    FloatingBall floating_ball;
    Bar bar;
    vector<Brick> bricks;

    // Initialize the LED wall, floating ball, bar, and bricks
    Mat led_wall;
    initialize_led_wall(led_wall);
    initialize_floating_ball(floating_ball);
    initialize_bar(bar);
    initialize_bricks(bricks);

    // Open the named pipe
    int pipe_fd = open("/tmp/movement_pipe", O_RDONLY | O_NONBLOCK);
    if (pipe_fd == -1) {
        printf("Error: Could not open named pipe\n");
        return -1;
    }

    while (true) {
        // Capture a new frame
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }

        // Resize the frame to match the LED PCB wall resolution
        resize(frame, frame, Size(DISPLAY_WIDTH, DISPLAY_HEIGHT), 0, 0, INTER_LINEAR);

        // Update the floating ball
        update_floating_ball(floating_ball, bar, bricks);

        // Read from the named pipe
        char buffer[256];
        ssize_t bytes_read = read(pipe_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            // Parse the coordinates from the buffer (example: "Pose 0, Keypoint 7: (123.4, 567.8)\n")
            float hand_x = 0.0;
            float hand_y = 0.0;
            sscanf(buffer, "Pose %*d, Keypoint %*d: (%f, %f)", &hand_x, &hand_y);
            // Update the bar based on hand position
            update_bar(bar, hand_x);
        }

        // Draw the LED wall
        draw_led_wall(led_wall, floating_ball, bar, bricks);

        // Display the LED wall simulation
        imshow("LED PCB Wall Simulation", led_wall);

        // Exit the loop on 'q' key press
        if (waitKey(30) == 'q') break;
    }

    // Close the named pipe
    close(pipe_fd);

    // Release the camera
    cap.release();
    destroyAllWindows();

    return 0;
}