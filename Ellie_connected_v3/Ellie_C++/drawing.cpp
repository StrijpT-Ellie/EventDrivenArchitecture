#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/opencv_video_inc.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include <random>

const int pixelated_width = 20;
const int pixelated_height = 20;

cv::Mat persistent_pixels = cv::Mat::zeros(pixelated_height, pixelated_width, CV_8UC3);
cv::Mat canvas = cv::Mat::zeros(pixelated_height, pixelated_width, CV_8UC3);

void enhance_contrast(cv::Mat &image) {
    cv::Mat lab_image;
    cv::cvtColor(image, lab_image, cv::COLOR_BGR2Lab);
    std::vector<cv::Mat> lab_planes(3);
    cv::split(lab_image, lab_planes);
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(3.0, cv::Size(8, 8));
    clahe->apply(lab_planes[0], lab_planes[0]);
    cv::merge(lab_planes, lab_image);
    cv::cvtColor(lab_image, image, cv::COLOR_Lab2BGR);
}

void quantize_colors(cv::Mat &image) {
    for (int i = 0; i < image.rows; i++) {
        for (int j = 0; j < image.cols; j++) {
            cv::Vec3b &pixel = image.at<cv::Vec3b>(i, j);
            for (int k = 0; k < 3; k++) {
                pixel[k] = pixel[k] > 127 ? 255 : 0;
            }
        }
    }
}

void apply_falling_sand(cv::Mat &pixels) {
    for (int y = pixelated_height - 2; y >= 0; y--) {
        for (int x = 0; x < pixelated_width; x++) {
            if (cv::sum(pixels.at<cv::Vec3b>(y, x))[0] > 0 && cv::sum(pixels.at<cv::Vec3b>(y + 1, x))[0] == 0) {
                pixels.at<cv::Vec3b>(y + 1, x) = pixels.at<cv::Vec3b>(y, x);
                pixels.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);
            }
        }
    }

    float fade_factor = 0.9;
    pixels *= fade_factor;
    cv::threshold(pixels, pixels, 5, 255, cv::THRESH_TOZERO);
}

cv::Vec3b get_random_color() {
    static std::default_random_engine generator;
    static std::uniform_int_distribution<int> distribution(0, 255);
    return cv::Vec3b(distribution(generator), distribution(generator), distribution(generator));
}

int main() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error opening video stream or file" << std::endl;
        return -1;
    }

    mediapipe::CalculatorGraph graph;
    mediapipe::Status status = graph.Initialize(/* Pass MediaPipe graph configuration */);
    if (!status.ok()) {
        std::cerr << "Failed to initialize the graph: " << status.message() << std::endl;
        return -1;
    }

    graph.StartRun({});

    cv::namedWindow("Persistent Pixels", cv::WINDOW_AUTOSIZE);

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty())
            break;

        enhance_contrast(frame);

        cv::Mat frame_rgb;
        cv::cvtColor(frame, frame_rgb, cv::COLOR_BGR2RGB);

        // Process the frame using MediaPipe
        auto output_packets = graph.Run(graph.GetInputSidePackets());
        auto& hand_landmarks_packet = output_packets["hand_landmarks"];
        auto hand_landmarks = hand_landmarks_packet.Get<mediapipe::NormalizedLandmarkList>();

        cv::Mat pixelated;
        cv::resize(frame, pixelated, cv::Size(pixelated_width, pixelated_height), 0, 0, cv::INTER_LINEAR);
        quantize_colors(pixelated);

        if (!hand_landmarks.landmark().empty()) {
            const auto& palm_base = hand_landmarks.landmark(0);
            int centerX = static_cast<int>(palm_base.x() * pixelated_width);
            int centerY = static_cast<int>(palm_base.y() * pixelated_height);
            centerX = std::max(0, std::min(pixelated_width - 1, centerX));
            centerY = std::max(0, std::min(pixelated_height - 1, centerY));
            cv::Vec3b random_color = get_random_color();
            int brush_size = 2;
            cv::circle(canvas, cv::Point(centerX, centerY), brush_size, random_color, -1);

            for (int dy = -brush_size; dy <= brush_size; dy++) {
                for (int dx = -brush_size; dx <= brush_size; dx++) {
                    if (0 <= centerY + dy && centerY + dy < pixelated_height && 0 <= centerX + dx && centerX + dx < pixelated_width) {
                        persistent_pixels.at<cv::Vec3b>(centerY + dy, centerX + dx) = random_color;
                    }
                }
            }
        }

        apply_falling_sand(persistent_pixels);

        cv::Mat combined;
        cv::addWeighted(pixelated, 0.7, canvas, 0.3, 0, combined);

        cv::imshow("Persistent Pixels", persistent_pixels);

        if (cv::waitKey(30) >= 0)
            break;
    }

    cap.release();
    cv::destroyAllWindows();
    graph.CloseInputStream("input_video");
    graph.WaitUntilDone();

    return 0;
}
