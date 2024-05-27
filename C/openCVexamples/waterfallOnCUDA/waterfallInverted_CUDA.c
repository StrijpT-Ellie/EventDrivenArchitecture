//This version transfers enhance contrast funciton computation to CUDA 

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

void quantize_colors(Mat &image) {
    image.forEach<Vec3b>([](Vec3b &pixel, const int * position) -> void {
        pixel[0] = pixel[0] > 127 ? 255 : 0;
        pixel[1] = pixel[1] > 127 ? 255 : 0;
        pixel[2] = pixel[2] > 127 ? 255 : 0;
    });
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

int main(int argc, char** argv) {
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        printf("Error: Could not open camera\n");
        return -1;
    }

    Mat frame, prev_frame, prev_gray, long_exposure_frame;
    bool motion_detected = false;

    cap >> prev_frame;
    cvtColor(prev_frame, prev_gray, COLOR_BGR2GRAY);
    GaussianBlur(prev_gray, prev_gray, Size(21, 21), 0);

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

        Mat gray_frame, frame_diff, threshold_frame;
        cvtColor(frame, gray_frame, COLOR_BGR2GRAY);
        GaussianBlur(gray_frame, gray_frame, Size(21, 21), 0);

        absdiff(prev_gray, gray_frame, frame_diff);
        threshold(frame_diff, threshold_frame, 25, 255, THRESH_BINARY);
        dilate(threshold_frame, threshold_frame, Mat(), Point(-1, -1), 2);

        motion_detected = sum(threshold_frame)[0] > 10000;
        prev_gray = gray_frame.clone();

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
