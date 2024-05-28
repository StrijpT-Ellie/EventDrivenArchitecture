#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudaimgproc.hpp>

#define LED_WIDTH 20
#define LED_HEIGHT 20

int main(int argc, char** argv) {
    // Open the default camera
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        printf("Error: Could not open camera\n");
        return -1;
    }

    // Set frame rate to reduce processing load
    cap.set(cv::CAP_PROP_FPS, 15);

    // Create a window and resize it
    cv::namedWindow("LED PCB Wall Simulation", cv::WINDOW_NORMAL);
    cv::resizeWindow("LED PCB Wall Simulation", 640, 480);

    cv::Mat frame;
    cv::cuda::GpuMat d_frame, d_resizedFrame;
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
        cv::cuda::resize(d_frame, d_resizedFrame, cv::Size(LED_WIDTH, LED_HEIGHT), 0, 0, cv::INTER_LINEAR);

        // Download the resized frame back to the CPU
        d_resizedFrame.download(frame);

        // Create a new image to represent the LED wall
        cv::Mat led_wall(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3, cv::Scalar(0, 0, 0));

        // Scale the resized frame back up for display purposes
        cv::resize(frame, led_wall, cv::Size(DISPLAY_WIDTH, DISPLAY_HEIGHT), 0, 0, cv::INTER_NEAREST);

        // Display the LED wall simulation
        cv::imshow("LED PCB Wall Simulation", led_wall);

        // Exit the loop on 'q' key press
        if (cv::waitKey(30) == 'q') break;
    }

    // Release the camera
    cap.release();
    cv::destroyAllWindows();

    return 0;
}
