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
#define MOTION_THRESHOLD 25
#define PIXEL_MOVE_DISTANCE 5

using namespace cv;
using namespace std;

struct Pixel {
    Point position;
    Scalar color;
};

void detect_motion(const Mat &current_frame, const Mat &previous_frame, Mat &motion_mask) {
    Mat current_gray, previous_gray;
    cvtColor(current_frame, current_gray, COLOR_BGR2GRAY);
    cvtColor(previous_frame, previous_gray, COLOR_BGR2GRAY);

    Mat diff_frame, threshold_frame;
    absdiff(previous_gray, current_gray, diff_frame);
    threshold(diff_frame, threshold_frame, MOTION_THRESHOLD, 255, THRESH_BINARY);

    motion_mask = threshold_frame.clone();
}

void update_pixels(vector<Pixel> &pixels, const Mat &motion_mask) {
    for (int y = 0; y < motion_mask.rows; y++) {
        for (int x = 0; x < motion_mask.cols; x++) {
            if (motion_mask.at<uchar>(y, x) > 0) {
                // Move the pixel randomly
                Pixel &pixel = pixels[y * LED_WIDTH + x];
                pixel.position.x += rand() % (2 * PIXEL_MOVE_DISTANCE + 1) - PIXEL_MOVE_DISTANCE;
                pixel.position.y += rand() % (2 * PIXEL_MOVE_DISTANCE + 1) - PIXEL_MOVE_DISTANCE;

                // Ensure the pixel stays within bounds
                pixel.position.x = max(0, min(DISPLAY_WIDTH - 1, pixel.position.x));
                pixel.position.y = max(0, min(DISPLAY_HEIGHT - 1, pixel.position.y));

                // Change the color of the pixel
                pixel.color = Scalar(0, 255, 0); // Change to green when motion is detected
            }
        }
    }
}

void draw_pixels(Mat &led_wall, const vector<Pixel> &pixels) {
    int led_size_x = (DISPLAY_WIDTH - (LED_WIDTH - 1) * LED_SPACING) / LED_WIDTH;
    int led_size_y = (DISPLAY_HEIGHT - (LED_HEIGHT - 1) * LED_SPACING) / LED_HEIGHT;

    // Draw the "LEDs" with spacing
    for (const auto &pixel : pixels) {
        Rect led_rect(pixel.position.x * (led_size_x + LED_SPACING), pixel.position.y * (led_size_y + LED_SPACING), led_size_x, led_size_y);
        rectangle(led_wall, led_rect, pixel.color, FILLED);
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
    namedWindow("Static Pixels Reacting to Motion", WINDOW_NORMAL);
    resizeWindow("Static Pixels Reacting to Motion", DISPLAY_WIDTH, DISPLAY_HEIGHT);

    Mat frame, previous_frame, motion_mask;
    vector<Pixel> pixels(LED_WIDTH * LED_HEIGHT);
    srand(time(0));

    // Initialize static pixels
    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            pixels[y * LED_WIDTH + x] = { Point(x * (DISPLAY_WIDTH / LED_WIDTH), y * (DISPLAY_HEIGHT / LED_HEIGHT)), Scalar(255, 0, 0) };
        }
    }

    while (true) {
        // Capture a new frame
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }

        // Detect motion if there is a previous frame
        if (!previous_frame.empty()) {
            detect_motion(frame, previous_frame, motion_mask);
            update_pixels(pixels, motion_mask);
        }

        // Store the current frame as previous frame for the next iteration
        previous_frame = frame.clone();

        // Create a new image to represent the LED wall with spacing
        Mat led_wall(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3, Scalar(0, 0, 0));

        // Draw the static pixels
        draw_pixels(led_wall, pixels);

        // Display the LED wall simulation
        imshow("Static Pixels Reacting to Motion", led_wall);

        // Exit the loop on 'q' key press
        if (waitKey(30) == 'q') break;
    }

    // Release the camera
    cap.release();
    destroyAllWindows();

    return 0;
}


//g++ -std=c++11 -x c++ -I/usr/include/opencv4 array_noVideo.c -L/usr/lib/aarch64-linux-gnu -L/usr/local/cuda-10.2/lib64 -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio -lopencv_cudaarithm -lopencv_cudawarping -lopencv_cudaimgproc -lopencv_cudaobjdetect -lopencv_cudafilters -o array_noVideo

