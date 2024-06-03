//g++ -o hand_movement_detector_2 hand_movement_detector_2.cpp `pkg-config --cflags --libs opencv4` -lopencv_core -lopencv_highgui -lopencv_videoio -lopencv_cudaimgproc -lopencv_cudabgsegm -lopencv_cudafilters -lopencv_cudaarithm


#include <opencv2/opencv.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/cudabgsegm.hpp>
#include <opencv2/cudaarithm.hpp>
#include <iostream>

using namespace cv;
using namespace cv::cuda;
using namespace std;

void detectAndDrawContours(Mat& frame, Mat& mask);

int main(int argc, char** argv)
{
    // Open default camera
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cerr << "Error: Could not open camera" << endl;
        return -1;
    }

    // Create background subtractor using CUDA
    Ptr<cuda::BackgroundSubtractorMOG2> bg_subtractor = cuda::createBackgroundSubtractorMOG2();

    Mat frame;
    GpuMat d_frame, d_fg_mask, d_fg_mask_cleaned;
    Mat fg_mask_cleaned;

    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        // Upload frame to GPU
        d_frame.upload(frame);

        // Apply background subtraction
        bg_subtractor->apply(d_frame, d_fg_mask);

        // Remove noise by applying morphological operations on CPU
        cuda::threshold(d_fg_mask, d_fg_mask_cleaned, 127, 255, THRESH_BINARY);
        d_fg_mask_cleaned.download(fg_mask_cleaned); // Download to CPU for morphology operations
        morphologyEx(fg_mask_cleaned, fg_mask_cleaned, MORPH_OPEN, getStructuringElement(MORPH_RECT, Size(5, 5)));

        // Detect and draw contours
        detectAndDrawContours(frame, fg_mask_cleaned);

        // Show the frames
        imshow("Frame", frame);

        if (waitKey(1) >= 0) break;
    }

    return 0;
}

void detectAndDrawContours(Mat& frame, Mat& mask)
{
    vector<vector<Point>> contours;
    findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < contours.size(); i++) {
        double area = contourArea(contours[i]);
        if (area > 5000) { // Filter out small areas
            drawContours(frame, contours, (int)i, Scalar(0, 255, 0), 2);
            Rect boundRect = boundingRect(contours[i]);
            rectangle(frame, boundRect.tl(), boundRect.br(), Scalar(0, 0, 255), 2);
        }
    }
}
