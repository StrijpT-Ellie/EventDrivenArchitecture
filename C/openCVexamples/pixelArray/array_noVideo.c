#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <vector>
#include <ctime>
#include <deque>

#define LED_WIDTH 20
#define LED_HEIGHT 20
#define DISPLAY_WIDTH 640
#define DISPLAY_HEIGHT 480
#define LED_SPACING 5
#define SETTLE_DURATION 300  // in frames, assuming 30 FPS this would be ~10 seconds
#define MAX_NEW_FLOATING_PIXELS 10
#define MAX_ACCUMULATION_LINES 20
#define FRAME_AVERAGE_COUNT 5
#define DEBOUNCE_FRAMES 3

using namespace cv;
using namespace std;

struct FloatingPixel {
    Point position;
    Scalar color;
    int settle_frames;
};

struct PixelState {
    int count;
    bool isRed;
};

void initialize_led_wall(Mat &led_wall) {
    led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);
    int led_size_x = (DISPLAY_WIDTH - (LED_WIDTH - 1) * LED_SPACING) / LED_WIDTH;
    int led_size_y = (DISPLAY_HEIGHT - (LED_HEIGHT - 1) * LED_SPACING) / LED_HEIGHT;
    Scalar green_color(0, 255, 0); // Green color

    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            Rect led_rect(x * (led_size_x + LED_SPACING), y * (led_size_y + LED_SPACING), led_size_x, led_size_y);
            rectangle(led_wall, led_rect, green_color, FILLED);
        }
    }
}

void detect_and_float_red_pixels(const Mat &frame, vector<FloatingPixel> &floating_pixels, vector<vector<PixelState>> &pixel_states) {
    Mat hsv_frame;
    cvtColor(frame, hsv_frame, COLOR_BGR2HSV);

    // Define a tighter red color range
    Scalar lower_red1 = Scalar(0, 200, 200);
    Scalar upper_red1 = Scalar(10, 255, 255);
    Scalar lower_red2 = Scalar(160, 200, 200);
    Scalar upper_red2 = Scalar(180, 255, 255);

    Mat mask1, mask2, red_mask;
    inRange(hsv_frame, lower_red1, upper_red1, mask1);
    inRange(hsv_frame, lower_red2, upper_red2, mask2);
    bitwise_or(mask1, mask2, red_mask);

    // Update pixel states based on red detection
    for (int y = 0; y < red_mask.rows; y++) {
        for (int x = 0; x < red_mask.cols; x++) {
            bool isRed = red_mask.at<uchar>(y, x) > 0;
            PixelState &state = pixel_states[y][x];

            if (isRed) {
                state.count++;
                if (state.count >= DEBOUNCE_FRAMES) {
                    state.isRed = true;
                    state.count = DEBOUNCE_FRAMES; // Cap the count to avoid overflow
                }
            } else {
                state.count--;
                if (state.count <= 0) {
                    state.isRed = false;
                    state.count = 0;
                }
            }
        }
    }

    // Find red pixels and add them to the floating pixels list
    int new_floating_pixels_count = 0;
    for (int y = 0; y < red_mask.rows; y++) {
        for (int x = 0; x < red_mask.cols; x++) {
            if (pixel_states[y][x].isRed) {
                if (new_floating_pixels_count < MAX_NEW_FLOATING_PIXELS) {
                    FloatingPixel fp;
                    fp.position = Point(x, y);
                    fp.color = Scalar(0, 0, 255); // Red color
                    fp.settle_frames = SETTLE_DURATION;
                    floating_pixels.push_back(fp);
                    new_floating_pixels_count++;
                } else {
                    return; // Stop adding new floating pixels for this frame
                }
            }
        }
    }
}

void update_floating_pixels(vector<FloatingPixel> &floating_pixels, const Size &frame_size, vector<vector<int>> &accumulated_pixels, vector<vector<int>> &settle_timers) {
    for (auto &fp : floating_pixels) {
        if (fp.position.y < frame_size.height - 1) {
            // Check if the pixel can fall further down
            if (accumulated_pixels[fp.position.y + 1][fp.position.x] == 0) {
                fp.position.y++;
            } else {
                accumulated_pixels[fp.position.y][fp.position.x] = 1;
                settle_timers[fp.position.y][fp.position.x] = SETTLE_DURATION;
                fp.settle_frames = 0;
            }
        } else {
            accumulated_pixels[fp.position.y][fp.position.x] = 1;
            settle_timers[fp.position.y][fp.position.x] = SETTLE_DURATION;
            fp.settle_frames = 0;
        }
    }

    // Remove settled pixels
    floating_pixels.erase(remove_if(floating_pixels.begin(), floating_pixels.end(),
                                    [](const FloatingPixel &fp) { return fp.settle_frames <= 0; }),
                          floating_pixels.end());

    // Update the timers and remove pixels that have been settled for too long
    for (int y = 0; y < frame_size.height; y++) {
        for (int x = 0; x < frame_size.width; x++) {
            if (accumulated_pixels[y][x] == 1) {
                if (--settle_timers[y][x] <= 0) {
                    accumulated_pixels[y][x] = 0;
                }
            }
        }
    }

    // Clear the top line if more than MAX_ACCUMULATION_LINES are accumulated
    for (int y = frame_size.height - MAX_ACCUMULATION_LINES; y < frame_size.height; ++y) {
        bool line_filled = true;
        for (int x = 0; x < frame_size.width; ++x) {
            if (accumulated_pixels[y][x] == 0) {
                line_filled = false;
                break;
            }
        }
        if (line_filled) {
            for (int x = 0; x < frame_size.width; ++x) {
                accumulated_pixels[y][x] = 0;
                settle_timers[y][x] = 0;
            }
        }
    }
}

void draw_led_wall(Mat &led_wall, const vector<FloatingPixel> &floating_pixels, const vector<vector<int>> &accumulated_pixels) {
    int led_size_x = (DISPLAY_WIDTH - (LED_WIDTH - 1) * LED_SPACING) / LED_WIDTH;
    int led_size_y = (DISPLAY_HEIGHT - (LED_HEIGHT - 1) * LED_SPACING) / LED_HEIGHT;

    // Clear the LED wall
    led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);
    Scalar green_color(0, 255, 0); // Green color

    // Draw the "LEDs" with spacing and set initial color to green
    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            Rect led_rect(x * (led_size_x + LED_SPACING), y * (led_size_y + LED_SPACING), led_size_x, led_size_y);
            rectangle(led_wall, led_rect, green_color, FILLED);
        }
    }

    // Draw floating pixels
    for (const auto &fp : floating_pixels) {
        Rect led_rect(fp.position.x * (led_size_x + LED_SPACING), fp.position.y * (led_size_y + LED_SPACING), led_size_x, led_size_y);
        rectangle(led_wall, led_rect, fp.color, FILLED);
    }

    // Draw accumulated pixels
    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            if (accumulated_pixels[y][x] == 1) {
                Rect led_rect(x * (led_size_x + LED_SPACING), y * (led_size_y + LED_SPACING), led_size_x, led_size_y);
                rectangle(led_wall, led_rect, Scalar(0, 0, 255), FILLED);
            }
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

    Mat frame;
    cuda::GpuMat d_frame, d_resizedFrame;
    vector<FloatingPixel> floating_pixels;
    vector<vector<int>> accumulated_pixels(LED_HEIGHT, vector<int>(LED_WIDTH, 0));
    vector<vector<int>> settle_timers(LED_HEIGHT, vector<int>(LED_WIDTH, 0));
    vector<vector<PixelState>> pixel_states(LED_HEIGHT, vector<PixelState>(LED_WIDTH, {0, false}));
    deque<Mat> frame_buffer;
    srand(time(0));

    // Initialize the LED wall
    Mat led_wall;
    initialize_led_wall(led_wall);

    while (true) {
        // Capture a new frame
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }

        // Add the frame to the buffer
        frame_buffer.push_back(frame.clone());
        if (frame_buffer.size() > FRAME_AVERAGE_COUNT) {
            frame_buffer.pop_front();
        }

        // Compute the average frame
        Mat avg_frame = Mat::zeros(frame.size(), frame.type());
        for (const Mat &f : frame_buffer) {
            avg_frame += f / FRAME_AVERAGE_COUNT;
        }

        // Upload the frame to the GPU
        d_frame.upload(avg_frame);

        // Resize the frame to match the LED PCB wall resolution using GPU
        cuda::resize(d_frame, d_resizedFrame, Size(LED_WIDTH, LED_HEIGHT), 0, 0, INTER_LINEAR);

        // Download the resized frame back to the CPU
        d_resizedFrame.download(avg_frame);

        // Detect red pixels and add floating effect
        detect_and_float_red_pixels(avg_frame, floating_pixels, pixel_states);

        // Update positions of floating pixels
        update_floating_pixels(floating_pixels, Size(LED_WIDTH, LED_HEIGHT), accumulated_pixels, settle_timers);

        // Draw the LED wall and floating pixels
        draw_led_wall(led_wall, floating_pixels, accumulated_pixels);

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
