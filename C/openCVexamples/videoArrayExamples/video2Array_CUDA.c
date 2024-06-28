#include <stdio.h>
#include </usr/include/opencv4/opencv2/opencv.hpp>
#include </usr/include/opencv4/opencv2/cudawarping.hpp>
#include </usr/include/opencv4/opencv2/cudaimgproc.hpp>

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
    cv::namedWindow("Pixelated Webcam", cv::WINDOW_NORMAL);
    cv::resizeWindow("Pixelated Webcam", 640, 480);

    cv::Mat frame;
    cv::cuda::GpuMat d_frame, d_resizedFrame, d_pixelatedFrame;
    while (true) {
        // Capture a new frame
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }

        // Upload the frame to the GPU
        d_frame.upload(frame);

        // Resize the frame to 10x10 using GPU (adjust as needed)
        cv::cuda::resize(d_frame, d_resizedFrame, cv::Size(10, 10), 0, 0, cv::INTER_LINEAR);

        // Resize it back to the original size to visualize the pixelated effect using GPU
        cv::cuda::resize(d_resizedFrame, d_pixelatedFrame, d_frame.size(), 0, 0, cv::INTER_NEAREST);

        // Download the pixelated frame back to the CPU
        d_pixelatedFrame.download(frame);

        // Display the pixelated frame
        cv::imshow("Pixelated Webcam", frame);

        // Exit the loop on 'q' key press
        if (cv::waitKey(30) >= 0) break;
    }

    // Release the camera
    cap.release();
    cv::destroyAllWindows();

    return 0;
}
