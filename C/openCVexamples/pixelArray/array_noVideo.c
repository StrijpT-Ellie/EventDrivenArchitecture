#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <ctime>

#define LED_WIDTH 20
#define LED_HEIGHT 20
#define DISPLAY_WIDTH 640
#define DISPLAY_HEIGHT 480
#define LED_SPACING 5

using namespace cv;
using namespace std;

struct Pixel {
    Point position;
    Scalar color;
};

void update_pixels_based_on_movement(const Mat &motion_mask, vector<vector<Pixel>> &pixels) {
    for (int y = 0; y < motion_mask.rows; y++) {
        for (int x = 0; x < motion_mask.cols; x++) {
            if (motion_mask.at<uchar>(y, x) > 0) {
                // Move pixel randomly based on detected motion
                int new_x = max(0, min(LED_WIDTH - 1, x + rand() % 3 - 1));
                int new_y = max(0, min(LED_HEIGHT - 1, y + rand() % 3 - 1));
                pixels[y][x].position = Point(new_x, new_y);

                // Change color based on motion
                pixels[y][x].color = Scalar(rand() % 256, rand() % 256, rand() % 256);
            }
        }
    }
}

void draw_led_wall(Mat &led_wall, const vector<vector<Pixel>> &pixels) {
    int led_size_x = (DISPLAY_WIDTH - (LED_WIDTH - 1) * LED_SPACING) / LED_WIDTH;
    int led_size_y = (DISPLAY_HEIGHT - (LED_HEIGHT - 1) * LED_SPACING) / LED_HEIGHT;

    // Draw the "LEDs" with spacing
    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            const Pixel &pixel = pixels[y][x];
            Rect led_rect(pixel.position.x * (led_size_x + LED_SPACING), pixel.position.y * (led_size_y + LED_SPACING), led_size_x, led_size_y);
            rectangle(led_wall, led_rect, pixel.color, FILLED);
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

    Mat frame, prev_frame, gray_frame, prev_gray_frame, frame_diff, motion_mask;
    srand(time(0));

    // Initialize pixels with random colors
    vector<vector<Pixel>> pixels(LED_HEIGHT, vector<Pixel>(LED_WIDTH));
    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            pixels[y][x] = { Point(x, y), Scalar(rand() % 256, rand() % 256, rand() % 256) };
        }
    }

    // Capture the first frame
    cap >> frame;
    cvtColor(frame, prev_gray_frame, COLOR_BGR2GRAY);

    while (true) {
        // Capture a new frame
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }

        // Convert frame to grayscale
        cvtColor(frame, gray_frame, COLOR_BGR2GRAY);

        // Compute the absolute difference between the current frame and the previous frame
        absdiff(gray_frame, prev_gray_frame, frame_diff);

        // Threshold the difference to get the motion mask
        threshold(frame_diff, motion_mask, 25, 255, THRESH_BINARY);

        // Update previous frame
        prev_gray_frame = gray_frame.clone();

        // Update pixels based on the motion mask
        update_pixels_based_on_movement(motion_mask, pixels);

        // Create a new image to represent the LED wall with spacing
        Mat led_wall(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3, Scalar(0, 0, 0));

        // Draw the LED wall
        draw_led_wall(led_wall, pixels);

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


//g++ -std=c++11 -x c++ -I/usr/include/opencv4 array_noVideo.c -L/usr/lib/aarch64-linux-gnu -L/usr/local/cuda-10.2/lib64 -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio -lopencv_cudaarithm -lopencv_cudawarping -lopencv_cudaimgproc -lopencv_cudaobjdetect -lopencv_cudafilters -o video2Array_LEDs_no_video
./array_noVideo
