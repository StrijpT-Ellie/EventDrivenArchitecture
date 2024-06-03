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

using namespace cv;
using namespace std;

struct FloatingCircle {
    Point2f position;
    Point2f velocity;
    int radius;
    Scalar color;
};

void initialize_led_wall(Mat &led_wall) {
    led_wall = Mat::zeros(DISPLAY_WIDTH, DISPLAY_HEIGHT, CV_8UC3);
}

void initialize_floating_circle(FloatingCircle &circle) {
    circle.position = Point2f(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
    circle.velocity = Point2f(3, 3);  // Velocity of the circle
    circle.radius = 20;
    circle.color = Scalar(0, 0, 255);  // Red color
}

void detect_movement(const Mat &prev_frame, const Mat &current_frame, vector<vector<bool>> &movement_map) {
    Mat gray_prev, gray_current, diff;
    cvtColor(prev_frame, gray_prev, COLOR_BGR2GRAY);
    cvtColor(current_frame, gray_current, COLOR_BGR2GRAY);
    absdiff(gray_prev, gray_current, diff);
    threshold(diff, diff, MOVEMENT_THRESHOLD, 255, THRESH_BINARY);

    int led_size_x = (DISPLAY_WIDTH - (LED_WIDTH - 1) * LED_SPACING) / LED_WIDTH;
    int led_size_y = (DISPLAY_HEIGHT - (LED_HEIGHT - 1) * LED_SPACING) / LED_HEIGHT;

    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            Rect led_rect(x * (led_size_x + LED_SPACING), y * (led_size_y + LED_SPACING), led_size_x, led_size_y);

            // Ensure the ROI stays within image bounds
            led_rect &= Rect(0, 0, diff.cols, diff.rows);

            Mat roi = diff(led_rect);
            movement_map[y][x] = countNonZero(roi) > 0;
        }
    }
}

void update_floating_circle(FloatingCircle &circle, const vector<vector<bool>> &movement_map) {
    circle.position += circle.velocity;

    // Check for collisions with the edges of the display
    if (circle.position.x - circle.radius < 0) {
        circle.velocity.x = abs(circle.velocity.x); // Bounce right
    } else if (circle.position.x + circle.radius > DISPLAY_WIDTH) {
        circle.velocity.x = -abs(circle.velocity.x); // Bounce left
    }
    if (circle.position.y - circle.radius < 0) {
        circle.velocity.y = abs(circle.velocity.y); // Bounce down
    } else if (circle.position.y + circle.radius > DISPLAY_HEIGHT) {
        circle.velocity.y = -abs(circle.velocity.y); // Bounce up
    }

    // Check for collisions with movement areas
    int led_size_x = (DISPLAY_WIDTH - (LED_WIDTH - 1) * LED_SPACING) / LED_WIDTH;
    int led_size_y = (DISPLAY_HEIGHT - (LED_HEIGHT - 1) * LED_SPACING) / LED_HEIGHT;
    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            if (movement_map[y][x]) {
                Rect led_rect(x * (led_size_x + LED_SPACING), y * (led_size_y + LED_SPACING), led_size_x, led_size_y);
                if (circle.position.x + circle.radius > led_rect.x &&
                    circle.position.x - circle.radius < led_rect.x + led_rect.width &&
                    circle.position.y + circle.radius > led_rect.y &&
                    circle.position.y - circle.radius < led_rect.y + led_rect.height) {
                    if (abs(circle.position.x - (led_rect.x + led_rect.width / 2)) > abs(circle.position.y - (led_rect.y + led_rect.height / 2))) {
                        circle.velocity.x = -circle.velocity.x;
                    } else {
                        circle.velocity.y = -circle.velocity.y;
                    }
                }
            }
        }
    }
}

void draw_led_wall(Mat &led_wall, const vector<vector<bool>> &movement_map, const FloatingCircle &circle) {
    int led_size_x = (DISPLAY_WIDTH - (LED_WIDTH - 1) * LED_SPACING) / LED_WIDTH;
    int led_size_y = (DISPLAY_HEIGHT - (LED_HEIGHT - 1) * LED_SPACING) / LED_HEIGHT;

    // Clear the LED wall
    led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);

    // Draw the movement areas
    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            Rect led_rect(x * (led_size_x + LED_SPACING), y * (led_size_y + LED_SPACING), led_size_x, led_size_y);
            if (movement_map[y][x]) {
                rectangle(led_wall, led_rect, Scalar(0, 255, 0), FILLED);  // Green color for movement areas
            } else {
                rectangle(led_wall, led_rect, Scalar(0, 0, 0), FILLED);  // Black color for non-movement areas
            }
        }
    }

    // Draw the floating circle
    cv::circle(led_wall, circle.position, circle.radius, circle.color, FILLED);
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
    FloatingCircle floating_circle;
    vector<vector<bool>> movement_map(LED_HEIGHT, vector<bool>(LED_WIDTH, false));

    // Initialize the LED wall and floating circle
    Mat led_wall;
    initialize_led_wall(led_wall);
    initialize_floating_circle(floating_circle);

    while (true) {
        // Capture a new frame
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }

        // Resize the frame to match the LED PCB wall resolution
        resize(frame, frame, Size(DISPLAY_WIDTH, DISPLAY_HEIGHT), 0, 0, INTER_LINEAR);

        // If there's a previous frame, detect movement
        if (!prev_frame.empty()) {
            detect_movement(prev_frame, frame, movement_map);
        }

        // Update the previous frame
        prev_frame = frame.clone();

        // Update the floating circle
        update_floating_circle(floating_circle, movement_map);

        // Draw the LED wall
        draw_led_wall(led_wall, movement_map, floating_circle);

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
