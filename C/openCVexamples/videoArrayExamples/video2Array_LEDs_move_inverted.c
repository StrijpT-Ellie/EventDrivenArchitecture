#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <vector>
#include <ctime>

#define LED_WIDTH 20
#define LED_HEIGHT 20
#define DISPLAY_WIDTH 640
#define DISPLAY_HEIGHT 480
#define LED_SPACING 5
#define FLOAT_DURATION 100

using namespace cv;
using namespace std;

struct FloatingPixel {
    Point position;
    Scalar color;
    int remaining_frames;
};

void quantize_colors(cv::Mat &image) {
    vector<Mat> channels;
    split(image, channels);
    for (int i = 0; i < 3; i++) {
        threshold(channels[i], channels[i], 127, 255, THRESH_BINARY);
    }
    merge(channels, image);
}

vector<FloatingPixel> detect_and_float_red_pixels(const Mat &frame, vector<FloatingPixel> &floating_pixels) {
    Mat hsv_frame;
    cvtColor(frame, hsv_frame, COLOR_BGR2HSV);

    // Define red color range
    Scalar lower_red1 = Scalar(0, 100, 100);
    Scalar upper_red1 = Scalar(10, 255, 255);
    Scalar lower_red2 = Scalar(160, 100, 100);
    Scalar upper_red2 = Scalar(180, 255, 255);

    Mat mask1, mask2, red_mask;
    inRange(hsv_frame, lower_red1, upper_red1, mask1);
    inRange(hsv_frame, lower_red2, upper_red2, mask2);
    bitwise_or(mask1, mask2, red_mask);

    // Find red pixels and add them to the floating pixels list
    for (int y = 0; y < red_mask.rows; y++) {
        for (int x = 0; x < red_mask.cols; x++) {
            if (red_mask.at<uchar>(y, x) > 0) {
                FloatingPixel fp;
                fp.position = Point(x, y);
                fp.color = Scalar(0, 0, 255); // Red color
                fp.remaining_frames = FLOAT_DURATION;
                floating_pixels.push_back(fp);
            }
        }
    }

    return floating_pixels;
}

void update_floating_pixels(vector<FloatingPixel> &floating_pixels, const Size &frame_size) {
    for (auto &fp : floating_pixels) {
        if (fp.remaining_frames > 0) {
            fp.position.x += rand() % 5 - 2;
            fp.position.y += rand() % 5 - 2;
            fp.position.x = max(0, min(frame_size.width - 1, fp.position.x));
            fp.position.y = max(0, min(frame_size.height - 1, fp.position.y));
            fp.remaining_frames--;
        }
    }

    // Remove expired floating pixels
    floating_pixels.erase(remove_if(floating_pixels.begin(), floating_pixels.end(),
                                    [](const FloatingPixel &fp) { return fp.remaining_frames <= 0; }),
                          floating_pixels.end());
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
    cuda::GpuMat d_frame, d_resizedFrame;
    vector<FloatingPixel> floating_pixels;
    srand(time(0));

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

        // Detect red pixels and add floating effect
        detect_and_float_red_pixels(frame, floating_pixels);

        // Update positions of floating pixels
        update_floating_pixels(floating_pixels, frame.size());

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

        // Draw floating pixels
        for (const auto &fp : floating_pixels) {
            Rect led_rect(fp.position.x * (led_size_x + LED_SPACING), fp.position.y * (led_size_y + LED_SPACING), led_size_x, led_size_y);
            rectangle(led_wall, led_rect, fp.color, FILLED);
        }

        // Invert the colors
        bitwise_not(led_wall, led_wall);

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
