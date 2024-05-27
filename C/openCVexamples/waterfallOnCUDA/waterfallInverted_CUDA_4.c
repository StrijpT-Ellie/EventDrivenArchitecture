//This version transfers enhance contrast funciton computation to CUDA 
//And also quantize colors is on GPU 
//GaussianBlur is also on GPU 
//absdiff and threshold are on GPU 
//cvtColor also on CUDA 

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <opencv2/opencv.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudacodec.hpp>
#include <opencv2/cudafilters.hpp>
#include <opencv2/cudaimgproc.hpp>

#define PIXELATED_WIDTH 20
#define PIXELATED_HEIGHT 20
#define DISPLAY_WIDTH 400
#define DISPLAY_HEIGHT 400

using namespace cv;
using namespace std;

// Function declarations
void quantize_colors(Mat &image);
void enhance_contrast(Mat &image);
void cudaGaussianBlur(const cv::cuda::GpuMat& src, cv::cuda::GpuMat& dst, cv::Size ksize, double sigmaX, double sigmaY = 0);

void quantize_colors(Mat &image) {
    cv::cuda::GpuMat gpu_image;
    gpu_image.upload(image);

    cv::cuda::GpuMat gpu_quantized;
    cv::cuda::threshold(gpu_image, gpu_quantized, 127, 255, cv::THRESH_BINARY);

    gpu_quantized.download(image);
}

void enhance_contrast(cv::Mat &image) {
    cv::cuda::GpuMat gpu_image;
    gpu_image.upload(image);

    cv::cuda::GpuMat lab_image;
    cv::cuda::cvtColor(gpu_image, lab_image, cv::COLOR_BGR2Lab);

    std::vector<cv::cuda::GpuMat> lab_planes(3);
    cv::cuda::split(lab_image, lab_planes);

    cv::Ptr<cv::cuda::CLAHE> clahe = cv::cuda::createCLAHE(3.0, cv::Size(8, 8));
    clahe->apply(lab_planes[0], lab_planes[0]);

    cv::cuda::merge(lab_planes, lab_image);
    cv::cuda::cvtColor(lab_image, gpu_image, cv::COLOR_Lab2BGR);

    gpu_image.download(image);
}

void cudaGaussianBlur(const cv::cuda::GpuMat& src, cv::cuda::GpuMat& dst, cv::Size ksize, double sigmaX, double sigmaY) {
    cv::Ptr<cv::cuda::Filter> filter = cv::cuda::createGaussianFilter(src.type(), dst.type(), ksize, sigmaX, sigmaY);
    filter->apply(src, dst);
}

int main(int argc, char** argv) {
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        printf("Error: Could not open camera\n");
        return -1;
    }

    Mat frame, prev_frame, prev_gray;
    cv::cuda::GpuMat gpu_frame, gpu_gray, gpu_prev_gray, gpu_blurred, gpu_diff, gpu_threshold, gpu_dilated;
    Mat long_exposure_frame;
    bool motion_detected = false;

    cap >> prev_frame;
    cvtColor(prev_frame, prev_gray, COLOR_BGR2GRAY);

    gpu_prev_gray.upload(prev_gray);
    cudaGaussianBlur(gpu_prev_gray, gpu_prev_gray, Size(21, 21), 0);

    srand(time(0));
    vector<Point> original_positions, pixel_positions;

    original_positions.reserve(PIXELATED_HEIGHT * PIXELATED_WIDTH);
    for (int i = 0; i < PIXELATED_HEIGHT; ++i) {
        for (int j = 0; j < PIXELATED_WIDTH; ++j) {
            original_positions.emplace_back(j, i);
        }
    }

    pixel_positions = original_positions;

    while (true) {
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }

        enhance_contrast(frame);

        gpu_frame.upload(frame);
        cv::cuda::GpuMat gpu_gray;
        cv::cuda::cvtColor(gpu_frame, gpu_gray, COLOR_BGR2GRAY);
        cudaGaussianBlur(gpu_gray, gpu_blurred, Size(21, 21), 0);

        // CUDA-accelerated absdiff
        cv::cuda::absdiff(gpu_prev_gray, gpu_blurred, gpu_diff);

        // CUDA-accelerated threshold
        cv::cuda::threshold(gpu_diff, gpu_threshold, 25, 255, THRESH_BINARY);

        cv::Ptr<cv::cuda::Filter> dilate_filter = cv::cuda::createMorphologyFilter(MORPH_DILATE, gpu_threshold.type(), Mat(), Point(-1, -1), 2);
        dilate_filter->apply(gpu_threshold, gpu_dilated);

        gpu_dilated.download(frame);
        motion_detected = sum(frame)[0] > 10000;

        gpu_prev_gray = gpu_blurred.clone();

        Mat resized_frame, pixelated_frame;
        resize(frame, resized_frame, Size(PIXELATED_WIDTH, PIXELATED_HEIGHT), 0, 0, INTER_LINEAR);

        quantize_colors(resized_frame);

        resize(resized_frame, pixelated_frame, Size(DISPLAY_WIDTH, DISPLAY_HEIGHT), 0, 0, INTER_NEAREST);

        if (long_exposure_frame.empty()) {
            long_exposure_frame = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_32FC3);
        }

        Mat canvas = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);
        int radius = DISPLAY_WIDTH / PIXELATED_WIDTH / 2;
        int spacing_x = DISPLAY_WIDTH / PIXELATED_WIDTH;
        int spacing_y = DISPLAY_HEIGHT / PIXELATED_HEIGHT;

        if (motion_detected) {
            for (auto &pos : pixel_positions) {
                pos.x += rand() % 5 - 2;
                pos.y += rand() % 5 - 2;
                pos.x = max(0, min(PIXELATED_WIDTH - 1, pos.x));
                pos.y = max(0, min(PIXELATED_HEIGHT - 1, pos.y));
            }
        } else {
            pixel_positions = original_positions;
        }

        for (const auto &pos : pixel_positions) {
            Vec3b color = resized_frame.at<Vec3b>(pos.y, pos.x);
            circle(canvas, Point(pos.x * spacing_x + spacing_x / 2, pos.y * spacing_y + spacing_y / 2), radius, Scalar(color[0], color[1], color[2]), -1);
        }

        Mat canvas_float;
        canvas.convertTo(canvas_float, CV_32FC3);
        Mat canvas_inverted;
        canvas_float.convertTo(canvas_inverted, CV_8UC3);
        bitwise_not(canvas_inverted, canvas_inverted);
        canvas_inverted.convertTo(canvas_float, CV_32FC3);

        addWeighted(long_exposure_frame, 0.95, canvas_float, 0.05, 0, long_exposure_frame);
        Mat combined_frame;
        long_exposure_frame.convertTo(combined_frame, CV_8UC3);
        addWeighted(combined_frame, 0.7, Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3), 0.3, 0, combined_frame);

        imshow("Pixelated", combined_frame);

        if (waitKey(1) == 'q') {
            break;
        }
    }

    cap.release();
    destroyAllWindows();

    return 0;
}
