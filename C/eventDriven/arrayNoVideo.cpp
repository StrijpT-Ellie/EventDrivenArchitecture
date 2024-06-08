#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <ctime>

#define LED_WIDTH 20
#define LED_HEIGHT 20
#define DISPLAY_WIDTH 640
#define DISPLAY_HEIGHT 480
#define LED_SPACING 5
#define SETTLE_DURATION 10
#define MAX_NEW_FLOATING_PIXELS 10
#define MAX_ACCUMULATION_LINES 20
#define FRAME_AVERAGE_COUNT 5
#define DEBOUNCE_FRAMES 3
#define MOVEMENT_THRESHOLD 30
#define RED_DURATION 10

using namespace cv;
using namespace std;

struct PixelState {
    Scalar color;
    int timer;
};

void initialize_led_wall(Mat &led_wall, vector<vector<PixelState>> &led_states) {
    led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);
    int led_size_x = (DISPLAY_WIDTH - (LED_WIDTH - 1) * LED_SPACING) / LED_WIDTH;
    int led_size_y = (DISPLAY_HEIGHT - (LED_HEIGHT - 1) * LED_SPACING) / LED_HEIGHT;
    Scalar green_color(0, 255, 0);

    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            Rect led_rect(x * (led_size_x + LED_SPACING), y * (led_size_y + LED_SPACING), led_size_x, led_size_y);
            rectangle(led_wall, led_rect, green_color, FILLED);
            led_states[y][x] = { green_color, 0 };
        }
    }
}

void detect_movement(const Mat &prev_frame, const Mat &current_frame, vector<vector<PixelState>> &led_states) {
    Mat gray_prev, gray_current, diff;
    cvtColor(prev_frame, gray_prev, COLOR_BGR2GRAY);
    cvtColor(current_frame, gray_current, COLOR_BGR2GRAY);
    absdiff(gray_prev, gray_current, diff);
    threshold(diff, diff, MOVEMENT_THRESHOLD, 255, THRESH_BINARY);

    int movement_detected = 0;
    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            if (diff.at<uchar>(y, x) > 0) {
                led_states[y][x].color = Scalar(0, 0, 255);
                led_states[y][x].timer = RED_DURATION;
                movement_detected = 1;
            }
        }
    }

    if (movement_detected) {
        int fd = open("/tmp/movement_pipe", O_WRONLY | O_NONBLOCK);
        if (fd != -1) {
            time_t now = time(0);
            char* dt = ctime(&now);
            write(fd, dt, strlen(dt));
            close(fd);
        }
        printf("Movement detected and written to pipe\n");
    }
}

void update_led_states(vector<vector<PixelState>> &led_states) {
    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            if (led_states[y][x].timer > 0) {
                led_states[y][x].timer--;
                if (led_states[y][x].timer == 0) {
                    led_states[y][x].color = Scalar(0, 255, 0);
                }
            }
        }
    }
}

void draw_led_wall(Mat &led_wall, const vector<vector<PixelState>> &led_states) {
    int led_size_x = (DISPLAY_WIDTH - (LED_WIDTH - 1) * LED_SPACING) / LED_WIDTH;
    int led_size_y = (DISPLAY_HEIGHT - (LED_HEIGHT - 1) * LED_SPACING) / LED_HEIGHT;

    led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);

    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            Rect led_rect(x * (led_size_x + LED_SPACING), y * (led_size_y + LED_SPACING), led_size_x, led_size_y);
            rectangle(led_wall, led_rect, led_states[y][x].color, FILLED);
        }
    }
}

int main(int argc, char** argv) {
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        printf("Error: Could not open camera\n");
        return -1;
    }

    cap.set(CAP_PROP_FPS, 15);

    namedWindow("LED PCB Wall Simulation", WINDOW_NORMAL);
    resizeWindow("LED PCB Wall Simulation", DISPLAY_WIDTH, DISPLAY_HEIGHT);

    Mat frame, prev_frame;
    cuda::GpuMat d_frame, d_resizedFrame;
    vector<vector<PixelState>> led_states(LED_HEIGHT, vector<PixelState>(LED_WIDTH, { Scalar(0, 255, 0), 0 }));

    Mat led_wall;
    initialize_led_wall(led_wall, led_states);

    while (true) {
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }

        flip(frame, frame, 1); // Flip the frame horizontally

        d_frame.upload(frame);
        cuda::resize(d_frame, d_resizedFrame, Size(LED_WIDTH, LED_HEIGHT), 0, 0, INTER_LINEAR);
        d_resizedFrame.download(frame);

        if (!prev_frame.empty()) {
            detect_movement(prev_frame, frame, led_states);
        }

        prev_frame = frame.clone();

        update_led_states(led_states);
        draw_led_wall(led_wall, led_states);

        imshow("LED PCB Wall Simulation", led_wall);

        if (waitKey(30) == 'q') break;
    }

    cap.release();
    destroyAllWindows();

    return 0;
}
