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

    Mat frame, fg_mask, fg_mask_cpu;
    GpuMat d_frame, d_fg_mask;

    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        // Upload frame to GPU
        d_frame.upload(frame);

        // Apply background subtraction
        bg_subtractor->apply(d_frame, d_fg_mask);

        // Download the result back to CPU
        d_fg_mask.download(fg_mask);

        // Remove noise by applying morphological operations
        cv::threshold(fg_mask, fg_mask_cpu, 127, 255, THRESH_BINARY);
        cv::morphologyEx(fg_mask_cpu, fg_mask_cpu, MORPH_OPEN, getStructuringElement(MORPH_RECT, Size(5, 5)));

        // Detect and draw contours
        detectAndDrawContours(frame, fg_mask_cpu);

        // Show the frames
        imshow("Frame", frame);
        imshow("Foreground Mask", fg_mask_cpu);

        if (waitKey(30) >= 0) break;
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
