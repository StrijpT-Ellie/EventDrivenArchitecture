#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <vector>
#include <deque>

#define LED_WIDTH 20
#define LED_HEIGHT 20
#define DISPLAY_WIDTH 640
#define DISPLAY_HEIGHT 480
#define LED_SPACING 5
#define SETTLE_DURATION 10  // in frames, assuming 30 FPS this would be ~10 seconds
#define MAX_NEW_FLOATING_PIXELS 10
#define MAX_ACCUMULATION_LINES 20
#define FRAME_AVERAGE_COUNT 5
#define DEBOUNCE_FRAMES 3
#define MOVEMENT_THRESHOLD 30  // Threshold to detect movement
#define RED_DURATION 10  // Duration for red pixels to stay red in frames (approx 10 seconds at 30 FPS)
#define MOVEMENT_DETECTION_DURATION 180  // Duration to detect continuous movement (6 seconds at 30 FPS)
#define MIN_ACTIVE_PIXELS 10  // Minimum number of active pixels to consider significant movement
#define CHECK_INTERVAL 15  // Number of frames between checks
#define REQUIRED_CONSECUTIVE_DETECTIONS 6  // Number of consecutive detections needed for mode selection

using namespace cv;
using namespace std;

struct PixelState {
    Scalar color;
    int timer;
};

void initialize_led_wall(Mat &led_wall, vector<vector<PixelState>> &led_states) {
    led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);
    int led_size_x = (DISPLAY_WIDTH - (LED_WIDTH - 1) * LED_SPACING) / LED_WIDTH;
    int led_size_y = (DISPLAY_HEIGHT - (LED_HEIGHT - 1) * LED_SPACING) / LED_HEIGHT;
    Scalar green_color(0, 255, 0); // Green color

    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            Rect led_rect(x * (led_size_x + LED_SPACING), y * (led_size_y + LED_SPACING), led_size_x, led_size_y);
            rectangle(led_wall, led_rect, green_color, FILLED);
            led_states[y][x] = { green_color, 0 };
        }
    }
}

void detect_movement(const Mat &prev_frame, const Mat &current_frame, vector<vector<PixelState>> &led_states, int &left_counter, int &right_counter) {
    Mat gray_prev, gray_current, diff;
    cvtColor(prev_frame, gray_prev, COLOR_BGR2GRAY);
    cvtColor(current_frame, gray_current, COLOR_BGR2GRAY);
    absdiff(gray_prev, gray_current, diff);
    threshold(diff, diff, MOVEMENT_THRESHOLD, 255, THRESH_BINARY);

    int left_movement = 0;
    int right_movement = 0;

    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            if (diff.at<uchar>(y, x) > 0) {
                led_states[y][x].color = Scalar(0, 0, 255); // Red color
                led_states[y][x].timer = RED_DURATION;
                if (x < LED_WIDTH / 2) {
                    left_movement++;
                } else {
                    right_movement++;
                }
            }
        }
    }

    if (left_movement > MIN_ACTIVE_PIXELS) {
        left_counter++;
    } else {
        left_counter = max(0, left_counter - 1);
    }

    if (right_movement > MIN_ACTIVE_PIXELS) {
        right_counter++;
    } else {
        right_counter = max(0, right_counter - 1);
    }
}

void update_led_states(vector<vector<PixelState>> &led_states) {
    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            if (led_states[y][x].timer > 0) {
                led_states[y][x].timer--;
                if (led_states[y][x].timer == 0) {
                    led_states[y][x].color = Scalar(0, 255, 0); // Green color
                }
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
    cuda::GpuMat d_frame, d_resizedFrame;
    vector<vector<PixelState>> led_states(LED_HEIGHT, vector<PixelState>(LED_WIDTH, { Scalar(0, 255, 0), 0 })); // Green color with timer 0

    // Initialize the LED wall
    Mat led_wall;
    initialize_led_wall(led_wall, led_states);

    // Create a CUDA stream for asynchronous processing
    cuda::Stream stream;

    int left_counter = 0;
    int right_counter = 0;
    int frame_count = 0;
    int left_detections = 0;
    int right_detections = 0;

    while (true) {
        // Capture a new frame
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }

        // Flip the frame horizontally
        flip(frame, frame, 1); 

        // Resize the frame to match the LED PCB wall resolution using GPU
        d_frame.upload(frame, stream);
        cuda::resize(d_frame, d_resizedFrame, Size(LED_WIDTH, LED_HEIGHT), 0, 0, INTER_LINEAR, stream);
        d_resizedFrame.download(frame, stream);
        stream.waitForCompletion();

        // If there's a previous frame, detect movement
        if (!prev_frame.empty()) {
            detect_movement(prev_frame, frame, led_states, left_counter, right_counter);
        }

        // Update the previous frame
        prev_frame = frame.clone();

        // Update the LED states
        update_led_states(led_states);

        // Draw the LED wall
        draw_led_wall(led_wall, led_states);

        // Display the LED wall simulation
        imshow("LED PCB Wall Simulation", led_wall);

        // Print counters every CHECK_INTERVAL frames and check for continuous detections
        frame_count++;
        if (frame_count % CHECK_INTERVAL == 0) {
            printf("Left counter: %d, Right counter: %d\n", left_counter, right_counter);

            if (left_counter >= CHECK_INTERVAL) {
                left_detections++;
                right_detections = 0;
            } else if (right_counter >= CHECK_INTERVAL) {
                right_detections++;
                left_detections = 0;
            } else {
                left_detections = 0;
                right_detections = 0;
            }

            if (left_detections >= REQUIRED_CONSECUTIVE_DETECTIONS) {
                printf("Mode selected: 1\n");
                break;
            } else if (right_detections >= REQUIRED_CONSECUTIVE_DETECTIONS) {
                printf("Mode selected: 2\n");
                break;
            }
        }

        // Exit the loop on 'q' key press
        if (waitKey(30) == 'q') break;
    }

    // Release the camera
    cap.release();
    destroyAllWindows();

    return 0;
}
