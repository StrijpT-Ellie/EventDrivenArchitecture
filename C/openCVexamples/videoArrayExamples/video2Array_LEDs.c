#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudaimgproc.hpp>

#define LED_WIDTH 20
#define LED_HEIGHT 20
#define DISPLAY_WIDTH 640
#define DISPLAY_HEIGHT 480
#define LED_SPACING 5

using namespace cv;
using namespace std;

void quantize_colors(cv::Mat &image) {
    vector<Mat> channels;
    split(image, channels);
    for (int i = 0; i < 3; i++) {
        threshold(channels[i], channels[i], 127, 255, THRESH_BINARY);
    }
    merge(channels, image);
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
    cuda::GpuMat d_frame, d_resizedFrame, d_quantizedFrame;
    while (true) {
        // Capture a new frame
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }

        // Upload the frame to the GPU
        d_frame.upload(frame);

        // Resize the frame to match the LED PCB wall resolution using GPU
        cuda::resize(d_frame, d_resizedFrame, Size(LED_WIDTH, LED_HEIGHT), 0, 0, INTER_LINEAR);

        // Download the resized frame back to the CPU
        d_resizedFrame.download(frame);

        // Quantize the colors
        quantize_colors(frame);

        // Upload the quantized frame back to the GPU
        d_quantizedFrame.upload(frame);

        // Create a new image to represent the LED wall with spacing
        Mat led_wall(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3, Scalar(0, 0, 0));

        int led_size_x = (DISPLAY_WIDTH - (LED_WIDTH - 1) * LED_SPACING) / LED_WIDTH;
        int led_size_y = (DISPLAY_HEIGHT - (LED_HEIGHT - 1) * LED_SPACING) / LED_HEIGHT;

        // Draw the "LEDs" with spacing
        for (int y = 0; y < LED_HEIGHT; y++) {
            for (int x = 0; x < LED_WIDTH; x++) {
                Rect led_rect(x * (led_size_x + LED_SPACING), y * (led_size_y + LED_SPACING), led_size_x, led_size_y);
                rectangle(led_wall, led_rect, Scalar(frame.at<Vec3b>(y, x)[0], frame.at<Vec3b>(y, x)[1], frame.at<Vec3b>(y, x)[2]), FILLED);
            }
        }

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
