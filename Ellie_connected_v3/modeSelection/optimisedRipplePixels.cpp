#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <vector>
#include <ctime>
#include <deque>
#include <cmath>
#include <random>
#include <fcntl.h> // Include for pipe operations
#include <unistd.h> // Include for pipe operations

#define LED_WIDTH 20
#define LED_HEIGHT 20
#define DISPLAY_WIDTH 640
#define DISPLAY_HEIGHT 480
#define LED_SPACING 5
#define MOVEMENT_THRESHOLD 30  // Threshold to detect movement
#define RIPPLE_DURATION 60  // Duration for ripple to fade out in frames (approx 2 seconds at 30 FPS)
#define MAX_RIPPLES 500  // Maximum number of ripples allowed at a time

using namespace cv;
using namespace std;

struct PixelState {
    Scalar color;
    Scalar startColor;
    Scalar endColor;
    int timer;
};

struct RippleEffect {
    Point center;
    int radius;
    int duration;
    Scalar startColor;
    Scalar endColor;
};

Scalar getRandomColor() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 255);
    return Scalar(dis(gen), dis(gen), dis(gen));
}

void initialize_led_wall(Mat &led_wall, vector<vector<PixelState>> &led_states) {
    led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);
    int led_size_x = (DISPLAY_WIDTH - (LED_WIDTH - 1) * LED_SPACING) / LED_WIDTH;
    int led_size_y = (DISPLAY_HEIGHT - (LED_HEIGHT - 1) * LED_SPACING) / LED_HEIGHT;
    Scalar green_color(0, 255, 0); // Green color

    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            Rect led_rect(x * (led_size_x + LED_SPACING), y * (led_size_y + LED_SPACING), led_size_x, led_size_y);
            rectangle(led_wall, led_rect, green_color, FILLED);
            led_states[y][x] = { green_color, green_color, green_color, 0 };
        }
    }
}

void detect_movement(const Mat &prev_frame, const Mat &current_frame, vector<RippleEffect> &ripple_effects) {
    Mat gray_prev, gray_current, diff;
    cvtColor(prev_frame, gray_prev, COLOR_BGR2GRAY);
    cvtColor(current_frame, gray_current, COLOR_BGR2GRAY);
    absdiff(gray_prev, gray_current, diff);
    threshold(diff, diff, MOVEMENT_THRESHOLD, 255, THRESH_BINARY);

    int movement_detected = 0;
    int active_ripples = ripple_effects.size();

    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            if (diff.at<uchar>(y, x) > 0 && active_ripples < MAX_RIPPLES) {
                RippleEffect ripple = { Point(x, y), 0, RIPPLE_DURATION, getRandomColor(), getRandomColor() };
                ripple_effects.push_back(ripple);
                movement_detected = 1;
                active_ripples++;
            }
        }
    }

    if (movement_detected) {
        int fd = open("/tmp/movement_pipe", O_WRONLY | O_NONBLOCK);
        if (fd != -1) {
            printf("Writing to pipe...\n");
            time_t now = time(0);
            char* dt = ctime(&now);
            write(fd, dt, strlen(dt));
            close(fd);
        }
        printf("Movement detected and written to pipe\n");
    }
}

void update_ripple_effects(vector<RippleEffect> &ripple_effects, vector<vector<PixelState>> &led_states) {
    for (auto &ripple : ripple_effects) {
        ripple.radius++;
        ripple.duration--;

        for (int y = 0; y < LED_HEIGHT; y++) {
            for (int x = 0; x < LED_WIDTH; x++) {
                int dist = sqrt(pow(ripple.center.x - x, 2) + pow(ripple.center.y - y, 2));
                if (dist <= ripple.radius) {
                    led_states[y][x].startColor = ripple.startColor;
                    led_states[y][x].endColor = ripple.endColor;
                    led_states[y][x].timer = ripple.duration;
                }
            }
        }
    }

    ripple_effects.erase(remove_if(ripple_effects.begin(), ripple_effects.end(),
                                   [](const RippleEffect &ripple) { return ripple.duration <= 0; }),
                         ripple_effects.end());
}

void update_led_states(vector<vector<PixelState>> &led_states) {
    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            if (led_states[y][x].timer > 0) {
                double ratio = static_cast<double>(led_states[y][x].timer) / RIPPLE_DURATION;
                led_states[y][x].color = led_states[y][x].startColor * ratio + led_states[y][x].endColor * (1 - ratio);
                led_states[y][x].timer--;
            }
        }
    }
}

void draw_led_wall(Mat &led_wall, const vector<vector<PixelState>> &led_states) {
    int led_size_x = (DISPLAY_WIDTH - (LED_WIDTH - 1) * LED_SPACING) / LED_WIDTH;
    int led_size_y = (DISPLAY_HEIGHT - (LED_HEIGHT - 1) * LED_SPACING) / LED_HEIGHT;

    // Clear the LED wall
    led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);

    // Draw the "LEDs" with spacing
    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            Rect led_rect(x * (led_size_x + LED_SPACING), y * (led_size_y + LED_SPACING), led_size_x, led_size_y);
            rectangle(led_wall, led_rect, led_states[y][x].color, FILLED);
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
    vector<vector<PixelState>> led_states(LED_HEIGHT, vector<PixelState>(LED_WIDTH, { Scalar(0, 255, 0), Scalar(0, 255, 0), Scalar(0, 255, 0), 0 })); // Green color with timer 0
    vector<RippleEffect> ripple_effects;

    // Initialize the LED wall
    Mat led_wall;
    initialize_led_wall(led_wall, led_states);

    while (true) {
        // Capture a new frame
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }
        
        // Flip the frame horizontally
        flip(frame, frame, 1); 

        // Resize the frame to match the LED PCB wall resolution
        resize(frame, frame, Size(LED_WIDTH, LED_HEIGHT), 0, 0, INTER_LINEAR);

        // If there's a previous frame, detect movement
        if (!prev_frame.empty()) {
            detect_movement(prev_frame, frame, ripple_effects);
        }

        // Update the previous frame
        prev_frame = frame.clone();

        // Update ripple effects
        update_ripple_effects(ripple_effects, led_states);

        // Update the LED states
        update_led_states(led_states);

        // Draw the LED wall
        draw_led_wall(led_wall, led_states);

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
