//To compile 
//g++ -std=c++11 -x c++ -I/usr/include/opencv4 video2Array_smallerWindow.c -L/usr/lib/aarch64-linux-gnu -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio -o video2Array_smallerWindow


#include <stdio.h>
#include </usr/include/opencv4/opencv2/opencv.hpp>

int main(int argc, char** argv) {
    // Open the default camera
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        printf("Error: Could not open camera\n");
        return -1;
    }

    // Create a window and resize it
    cv::namedWindow("Pixelated Webcam", cv::WINDOW_NORMAL);
    cv::resizeWindow("Pixelated Webcam", 640, 480);

    cv::Mat frame;
    while (true) {
        // Capture a new frame
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }

        // Resize the frame to 10x10 (adjust as needed)
        cv::Mat resizedFrame;
        cv::resize(frame, resizedFrame, cv::Size(20, 20), 0, 0, cv::INTER_LINEAR);

        // Resize it back to the original size to visualize the pixelated effect
        cv::Mat pixelatedFrame;
        cv::resize(resizedFrame, pixelatedFrame, frame.size(), 0, 0, cv::INTER_NEAREST);

        // Display the pixelated frame
        cv::imshow("Pixelated Webcam", pixelatedFrame);

        // Exit the loop on 'q' key press
        if (cv::waitKey(30) >= 0) break;
    }

    // Release the camera
    cap.release();
    cv::destroyAllWindows();

    return 0;
}

