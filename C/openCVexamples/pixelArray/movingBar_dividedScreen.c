#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>

#define LED_WIDTH 20
#define LED_HEIGHT 20
#define DISPLAY_WIDTH 640
#define DISPLAY_HEIGHT 480
#define LED_SPACING 5
#define MOVEMENT_THRESHOLD 30  // Threshold to detect movement
#define BAR_HEIGHT 20  // Height of the bar at the bottom

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

void initialize_led_wall(Mat &led_wall) {
    led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);
}

void initialize_floating_ball(FloatingBall &ball) {
    ball.position = Point2f(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
    ball.velocity = Point2f(10, 10);  // Velocity of the ball
    ball.radius = 10;
    ball.color = Scalar(0, 0, 255);  // Red color
}

void initialize_bar(Bar &bar) {
    bar.position = Point2f(0, DISPLAY_HEIGHT - BAR_HEIGHT);
    bar.width = 100;
    bar.height = BAR_HEIGHT;
    bar.color = Scalar(255, 0, 0);  // Blue color
}

void detect_movement(const Mat &prev_frame, const Mat &current_frame, int &left_movement_intensity, int &right_movement_intensity) {
    Mat gray_prev, gray_current, diff;
    cvtColor(prev_frame, gray_prev, COLOR_BGR2GRAY);
    cvtColor(current_frame, gray_current, COLOR_BGR2GRAY);
    absdiff(gray_prev, gray_current, diff);
    threshold(diff, diff, MOVEMENT_THRESHOLD, 255, THRESH_BINARY);

    Mat left_half = diff(Rect(0, 0, diff.cols / 2, diff.rows));
    Mat right_half = diff(Rect(diff.cols / 2, 0, diff.cols / 2, diff.rows));

    left_movement_intensity = countNonZero(left_half);
    right_movement_intensity = countNonZero(right_half);
}

void update_floating_ball(FloatingBall &ball, const Bar &bar) {
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
}

void update_bar(Bar &bar, int left_movement_intensity, int right_movement_intensity) {
    if (right_movement_intensity > 0) {
        bar.position.x -= right_movement_intensity / 500.0; // Move left
    }
    if (left_movement_intensity > 0) {
        bar.position.x += left_movement_intensity / 500.0; // Move right
    }

    // Clamp the bar's position within the screen bounds
    if (bar.position.x < 0) bar.position.x = 0;
    if (bar.position.x + bar.width > DISPLAY_WIDTH) bar.position.x = DISPLAY_WIDTH - bar.width;
}

void draw_led_wall(Mat &led_wall, const FloatingBall &ball, const Bar &bar) {
    // Clear the LED wall
    led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);

    // Draw the floating ball
    cv::circle(led_wall, ball.position, ball.radius, ball.color, FILLED);

    // Draw the bar
    rectangle(led_wall, Rect(bar.position.x, bar.position.y, bar.width, bar.height), bar.color, FILLED);
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
    int left_movement_intensity = 0;
    int right_movement_intensity = 0;

    // Initialize the LED wall, floating ball, and bar
    Mat led_wall;
    initialize_led_wall(led_wall);
    initialize_floating_ball(floating_ball);
    initialize_bar(bar);

    while (true) {
        // Capture a new frame
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }

        // Flip the frame horizontally to correct the mirrored view
        flip(frame, frame, 1);

        // Resize the frame to match the LED PCB wall resolution
        resize(frame, frame, Size(DISPLAY_WIDTH, DISPLAY_HEIGHT), 0, 0, INTER_LINEAR);

        // If there's a previous frame, detect movement
        if (!prev_frame.empty()) {
            detect_movement(prev_frame, frame, left_movement_intensity, right_movement_intensity);
        }

        // Update the previous frame
        prev_frame = frame.clone();

        // Update the floating ball
        update_floating_ball(floating_ball, bar);

        // Update the bar based on movement intensity
        update_bar(bar, left_movement_intensity, right_movement_intensity);

        // Draw the LED wall
        draw_led_wall(led_wall, floating_ball, bar);

        // Display the LED wall simulation
        imshow("LED PCB Wall Simulation", led_wall);

        // Exit the loop on 'q' key press
        if (waitKey(30) == 'q') break;
    }

    // Release the camera
    cap.release();
    destroyAllWindows();

    return 0;
}
