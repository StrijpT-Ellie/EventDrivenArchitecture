#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <vector>
#include <deque>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define LED_WIDTH 20
#define LED_HEIGHT 20
#define DISPLAY_WIDTH 640
#define DISPLAY_HEIGHT 480
#define LED_SPACING 5
#define SETTLE_DURATION 10  // in frames, assuming 30 FPS this would be ~10 seconds
#define MAX_NEW_FLOATING_PIXELS 10
#define MAX_ACCUMULATION_LINES 20
#define FRAME_AVERAGE_COUNT 5
#define DEBOUNCE_FRAMES 3
#define MOVEMENT_THRESHOLD 30  // Threshold to detect movement
#define RED_DURATION 10  // Duration for red pixels to stay red in frames (approx 10 seconds at 30 FPS)
#define MOVEMENT_DETECTION_DURATION 180  // Duration to detect continuous movement (6 seconds at 30 FPS)
#define MIN_ACTIVE_PIXELS 10  // Minimum number of active pixels to consider significant movement
#define CHECK_INTERVAL 15  // Number of frames between checks
#define REQUIRED_CONSECUTIVE_DETECTIONS 6  // Number of consecutive detections needed for mode selection

#define BAR_HEIGHT 20  // Height of the bar at the bottom
#define BRICK_ROWS 5
#define BRICK_COLS 10
#define BRICK_WIDTH (DISPLAY_WIDTH / BRICK_COLS)
#define BRICK_HEIGHT 20

#define NUM_FOOD_PARTICLES 10  // Number of food particles
#define GRID_SIZE 20  // Size of each grid cell

using namespace cv;
using namespace std;

struct PixelState {
    Scalar color;
    int timer;
};

struct FloatingBall {
    Point2f position;
    Point2f velocity;
    int radius;
    Scalar color;
};

struct Bar {
    Point2f position;
    int width;
    int height;
    Scalar color;
};

struct Brick {
    Point2f position;
    int width;
    int height;
    Scalar color;
    bool active;
};

struct Particle {
    Point2f position;
    int radius;
    Scalar color;
};

struct Snake {
    vector<Point2f> body;
    Point2f velocity;
    int radius;
    Scalar color;
    int segments_to_add; // Number of segments to add when a particle is eaten
};

// Function Declarations
void write_movement_to_pipe(const char *pipe_path);
void detect_movement(const Mat &prev_frame, const Mat &current_frame, vector<vector<PixelState>> &led_states, int &left_counter, int &right_counter, const char *pipe_path);
void update_led_states(vector<vector<PixelState>> &led_states);
void draw_led_wall(Mat &led_wall, const vector<vector<PixelState>> &led_states);
void initialize_led_wall(Mat &led_wall, vector<vector<PixelState>> &led_states);
int mode_selector(const char *pipe_path);
void brickPong(const char *pipe_path);
void snake8Bit(const char *pipe_path);

void create_pipe(const char *pipe_path) {
    if (mkfifo(pipe_path, 0666) == -1) {
        perror("mkfifo failed");
    } else {
        printf("Named pipe created at %s\n", pipe_path);
    }
}

void write_movement_to_pipe(const char *pipe_path) {
    int fd = open(pipe_path, O_WRONLY | O_NONBLOCK);
    if (fd != -1) {
        time_t now = time(0);
        char *dt = ctime(&now);
        write(fd, dt, strlen(dt));
        close(fd);
        printf("Movement detected and written to pipe: %s\n", dt);
    } else {
        perror("open pipe for writing failed");
    }
}

void initialize_led_wall(Mat &led_wall, vector<vector<PixelState>> &led_states) {
    led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);
    int led_size_x = (DISPLAY_WIDTH - (LED_WIDTH - 1) * LED_SPACING) / LED_WIDTH;
    int led_size_y = (DISPLAY_HEIGHT - (LED_HEIGHT - 1) * LED_SPACING) / LED_HEIGHT;
    Scalar green_color(0, 255, 0); // Green color

    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            Rect led_rect(x * (led_size_x + LED_SPACING), y * (led_size_y + LED_SPACING), led_size_x, led_size_y);
            rectangle(led_wall, led_rect, green_color, FILLED);
            led_states[y][x] = { green_color, 0 };
        }
    }
}

void detect_movement(const Mat &prev_frame, const Mat &current_frame, vector<vector<PixelState>> &led_states, int &left_counter, int &right_counter, const char *pipe_path) {
    Mat gray_prev, gray_current, diff;
    cvtColor(prev_frame, gray_prev, COLOR_BGR2GRAY);
    cvtColor(current_frame, gray_current, COLOR_BGR2GRAY);
    absdiff(gray_prev, gray_current, diff);
    threshold(diff, diff, MOVEMENT_THRESHOLD, 255, THRESH_BINARY);

    int left_movement = 0;
    int right_movement = 0;

    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            if (diff.at<uchar>(y, x) > 0) {
                led_states[y][x].color = Scalar(0, 0, 255); // Red color
                led_states[y][x].timer = RED_DURATION;
                if (x < LED_WIDTH / 2) {
                    left_movement++;
                } else {
                    right_movement++;
                }
            }
        }
    }

    if (left_movement > MIN_ACTIVE_PIXELS) {
        left_counter++;
    } else {
        left_counter = max(0, left_counter - 1);
    }

    if (right_movement > MIN_ACTIVE_PIXELS) {
        right_counter++;
    } else {
        right_counter = max(0, right_counter - 1);
    }

    if (left_movement > MIN_ACTIVE_PIXELS || right_movement > MIN_ACTIVE_PIXELS) {
        printf("Movement detected: left=%d, right=%d\n", left_movement, right_movement);
        write_movement_to_pipe(pipe_path);
    }
}

void update_led_states(vector<vector<PixelState>> &led_states) {
    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            if (led_states[y][x].timer > 0) {
                led_states[y][x].timer--;
                if (led_states[y][x].timer == 0) {
                    led_states[y][x].color = Scalar(0, 255, 0); // Green color
                }
            }
        }
    }
}

void draw_led_wall(Mat &led_wall, const vector<vector<PixelState>> &led_states) {
    int led_size_x = (DISPLAY_WIDTH - (LED_WIDTH - 1) * LED_SPACING) / LED_WIDTH;
    int led_size_y = (DISPLAY_HEIGHT - (LED_HEIGHT - 1) * LED_SPACING) / LED_HEIGHT;

    // Clear the LED wall
    led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);

    // Draw the "LEDs" with spacing
    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            Rect led_rect(x * (led_size_x + LED_SPACING), y * (led_size_y + LED_SPACING), led_size_x, led_size_y);
            rectangle(led_wall, led_rect, led_states[y][x].color, FILLED);
        }
    }
}

int mode_selector(const char *pipe_path) {
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

    Mat frame, prev_frame;
    cuda::GpuMat d_frame, d_resizedFrame;
    vector<vector<PixelState>> led_states(LED_HEIGHT, vector<PixelState>(LED_WIDTH, { Scalar(0, 255, 0), 0 })); // Green color with timer 0

    // Initialize the LED wall
    Mat led_wall;
    initialize_led_wall(led_wall, led_states);

    // Create a CUDA stream for asynchronous processing
    cuda::Stream stream;

    int left_counter = 0;
    int right_counter = 0;
    int frame_count = 0;
    int left_detections = 0;
    int right_detections = 0;

    while (true) {
        // Capture a new frame
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }

        // Flip the frame horizontally
        flip(frame, frame, 1); 

        // Resize the frame to match the LED PCB wall resolution using GPU
        d_frame.upload(frame, stream);
        cuda::resize(d_frame, d_resizedFrame, Size(LED_WIDTH, LED_HEIGHT), 0, 0, INTER_LINEAR, stream);
        d_resizedFrame.download(frame, stream);
        stream.waitForCompletion();

        // If there's a previous frame, detect movement
        if (!prev_frame.empty()) {
            detect_movement(prev_frame, frame, led_states, left_counter, right_counter, pipe_path);
        }

        // Update the previous frame
        prev_frame = frame.clone();

        // Update the LED states
        update_led_states(led_states);

        // Draw the LED wall
        draw_led_wall(led_wall, led_states);

        // Display the LED wall simulation
        imshow("LED PCB Wall Simulation", led_wall);

        // Print counters every CHECK_INTERVAL frames and check for continuous detections
        frame_count++;
        if (frame_count % CHECK_INTERVAL == 0) {
            printf("Left counter: %d, Right counter: %d\n", left_counter, right_counter);

            if (left_counter >= CHECK_INTERVAL) {
                left_detections++;
                right_detections = 0;
            } else if (right_counter >= CHECK_INTERVAL) {
                right_detections++;
                left_detections = 0;
            } else {
                left_detections = 0;
                right_detections = 0;
            }

            if (left_detections >= REQUIRED_CONSECUTIVE_DETECTIONS) {
                printf("Mode selected: 1\n");
                cap.release();
                destroyAllWindows();
                return 1;
            } else if (right_detections >= REQUIRED_CONSECUTIVE_DETECTIONS) {
                printf("Mode selected: 2\n");
                cap.release();
                destroyAllWindows();
                return 2;
            }
        }

        // Exit the loop on 'q' key press
        if (waitKey(30) == 'q') break;
    }

    // Release the camera
    cap.release();
    destroyAllWindows();

    return -1;
}

void brickPong(const char *pipe_path) {
    // Open the default camera
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        printf("Error: Could not open camera\n");
        return;
    }

    // Set frame rate to reduce processing load
    cap.set(CAP_PROP_FPS, 15);

    // Create a window and resize it
    namedWindow("BrickPong", WINDOW_NORMAL);
    resizeWindow("BrickPong", DISPLAY_WIDTH, DISPLAY_HEIGHT);

    Mat frame, prev_frame;
    FloatingBall floating_ball;
    Bar bar;
    vector<Brick> bricks;
    int left_movement_intensity = 0;
    int right_movement_intensity = 0;

    // Initialize the LED wall, floating ball, bar, and bricks
    Mat led_wall;
    led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);
    floating_ball.position = Point2f(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
    floating_ball.velocity = Point2f(10, 10);  // Velocity of the ball
    floating_ball.radius = 20;
    floating_ball.color = Scalar(0, 0, 255);  // Red color
    bar.position = Point2f(0, DISPLAY_HEIGHT - BAR_HEIGHT);
    bar.width = 100;
    bar.height = BAR_HEIGHT;
    bar.color = Scalar(255, 0, 0);  // Blue color
    bricks.clear();
    for (int i = 0; i < BRICK_ROWS; i++) {
        for (int j = 0; j < BRICK_COLS; j++) {
            Brick brick;
            brick.position = Point2f(j * BRICK_WIDTH, i * BRICK_HEIGHT);
            brick.width = BRICK_WIDTH;
            brick.height = BRICK_HEIGHT;
            brick.color = Scalar(0, 255, 0);  // Green color
            brick.active = true;
            bricks.push_back(brick);
        }
    }

    while (true) {
        // Capture a new frame
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }

        // Flip the frame horizontally to correct the mirrored view
        //flip(frame, frame, 1);

        // Resize the frame to match the LED PCB wall resolution
        resize(frame, frame, Size(DISPLAY_WIDTH, DISPLAY_HEIGHT), 0, 0, INTER_LINEAR);

        // If there's a previous frame, detect movement
        if (!prev_frame.empty()) {
            Mat gray_prev, gray_current, diff;
            cvtColor(prev_frame, gray_prev, COLOR_BGR2GRAY);
            cvtColor(frame, gray_current, COLOR_BGR2GRAY);
            absdiff(gray_prev, gray_current, diff);
            threshold(diff, diff, MOVEMENT_THRESHOLD, 255, THRESH_BINARY);

            Mat left_half = diff(Rect(0, 0, diff.cols / 2, diff.rows));
            Mat right_half = diff(Rect(diff.cols / 2, 0, diff.cols / 2, diff.rows));

            left_movement_intensity = countNonZero(left_half);
            right_movement_intensity = countNonZero(right_half);

            if (left_movement_intensity > MIN_ACTIVE_PIXELS || right_movement_intensity > MIN_ACTIVE_PIXELS) {
                printf("BrickPong movement detected: left=%d, right=%d\n", left_movement_intensity, right_movement_intensity);
                write_movement_to_pipe(pipe_path);
            }
        }

        // Update the previous frame
        prev_frame = frame.clone();

        // Update the floating ball
        floating_ball.position += floating_ball.velocity;

        // Check for collisions with the edges of the display
        if (floating_ball.position.x - floating_ball.radius < 0) {
            floating_ball.velocity.x = abs(floating_ball.velocity.x); // Bounce right
        } else if (floating_ball.position.x + floating_ball.radius > DISPLAY_WIDTH) {
            floating_ball.velocity.x = -abs(floating_ball.velocity.x); // Bounce left
        }
        if (floating_ball.position.y - floating_ball.radius < 0) {
            floating_ball.velocity.y = abs(floating_ball.velocity.y); // Bounce down
        } else if (floating_ball.position.y + floating_ball.radius > DISPLAY_HEIGHT) {
            floating_ball.velocity.y = -abs(floating_ball.velocity.y); // Bounce up
        }

        // Check for collisions with the bar
        Rect bar_rect(bar.position, Size(bar.width, bar.height));
        if (floating_ball.position.x + floating_ball.radius > bar_rect.x &&
            floating_ball.position.x - floating_ball.radius < bar_rect.x + bar_rect.width &&
            floating_ball.position.y + floating_ball.radius > bar_rect.y &&
            floating_ball.position.y - floating_ball.radius < bar_rect.y + bar_rect.height) {
            if (floating_ball.position.y + floating_ball.radius >= bar_rect.y && floating_ball.position.y - floating_ball.radius < bar_rect.y) {
                floating_ball.velocity.y = -abs(floating_ball.velocity.y); // Bounce up
            }
        }

        // Check for collisions with bricks
        for (auto &brick : bricks) {
            if (brick.active) {
                Rect brick_rect(brick.position, Size(brick.width, brick.height));
                if (floating_ball.position.x + floating_ball.radius > brick_rect.x &&
                    floating_ball.position.x - floating_ball.radius < brick_rect.x + brick_rect.width &&
                    floating_ball.position.y + floating_ball.radius > brick_rect.y &&
                    floating_ball.position.y - floating_ball.radius < brick_rect.y + brick_rect.height) {
                    brick.active = false;
                    if (abs(floating_ball.position.x - (brick_rect.x + brick_rect.width / 2)) > abs(floating_ball.position.y - (brick_rect.y + brick_rect.height / 2))) {
                        floating_ball.velocity.x = -floating_ball.velocity.x;
                    } else {
                        floating_ball.velocity.y = -floating_ball.velocity.y;
                    }
                }
            }
        }

        // Update the bar based on movement intensity
        if (right_movement_intensity > 0) {
            bar.position.x -= right_movement_intensity / 500.0; // Move left
        }
        if (left_movement_intensity > 0) {
            bar.position.x += left_movement_intensity / 500.0; // Move right
        }

        // Clamp the bar's position within the screen bounds
        if (bar.position.x < 0) bar.position.x = 0;
        if (bar.position.x + bar.width > DISPLAY_WIDTH) bar.position.x = DISPLAY_WIDTH - bar.width;

        // Clear the LED wall
        led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);

        // Draw the floating ball
        cv::circle(led_wall, floating_ball.position, floating_ball.radius, floating_ball.color, FILLED);

        // Draw the bar
        rectangle(led_wall, Rect(bar.position.x, bar.position.y, bar.width, bar.height), bar.color, FILLED);

        // Draw the bricks
        for (const auto &brick : bricks) {
            if (brick.active) {
                rectangle(led_wall, Rect(brick.position.x, brick.position.y, brick.width, brick.height), brick.color, FILLED);
            }
        }

        // Display the LED wall simulation
        imshow("BrickPong", led_wall);

        // Exit the loop on 'q' key press
        if (waitKey(30) == 'q') break;
    }

    // Release the camera
    cap.release();
    destroyAllWindows();
}

void snake8Bit(const char *pipe_path) {
    // Open the default camera
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        printf("Error: Could not open camera\n");
        return;
    }

    // Set frame rate to reduce processing load
    cap.set(CAP_PROP_FPS, 15);

    // Create a window and resize it
    namedWindow("Snake8Bit", WINDOW_NORMAL);
    resizeWindow("Snake8Bit", DISPLAY_WIDTH, DISPLAY_HEIGHT);

    Mat frame, prev_frame;
    Snake snake;
    vector<Particle> food_particles;
    int left_movement_intensity = 0;
    int right_movement_intensity = 0;

    // Initialize the LED wall, snake, and food particles
    Mat led_wall;
    led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);
    snake.body.push_back(Point2f(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2));
    snake.velocity = Point2f(3, 0);  // Initial velocity to the right
    snake.radius = 10;
    snake.color = Scalar(0, 0, 255);  // Red color
    snake.segments_to_add = 0;  // Initialize with no segments to add
    srand(time(0));
    for (int i = 0; i < NUM_FOOD_PARTICLES; ++i) {
        Particle food;
        food.position = Point2f(rand() % DISPLAY_WIDTH, rand() % DISPLAY_HEIGHT);
        food.radius = 5;
        food.color = Scalar(0, 255, 0);  // Green color
        food_particles.push_back(food);
    }

    while (true) {
        // Capture a new frame
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }

        // Flip the frame horizontally to correct the mirrored view
        //flip(frame, frame, 1);

        // Resize the frame to match the LED PCB wall resolution
        resize(frame, frame, Size(DISPLAY_WIDTH, DISPLAY_HEIGHT), 0, 0, INTER_LINEAR);

        // If there's a previous frame, detect movement
        if (!prev_frame.empty()) {
            Mat gray_prev, gray_current, diff;
            cvtColor(prev_frame, gray_prev, COLOR_BGR2GRAY);
            cvtColor(frame, gray_current, COLOR_BGR2GRAY);
            absdiff(gray_prev, gray_current, diff);
            threshold(diff, diff, MOVEMENT_THRESHOLD, 255, THRESH_BINARY);

            Mat left_half = diff(Rect(0, 0, diff.cols / 2, diff.rows));
            Mat right_half = diff(Rect(diff.cols / 2, 0, diff.cols / 2, diff.rows));

            left_movement_intensity = countNonZero(left_half);
            right_movement_intensity = countNonZero(right_half);

            if (left_movement_intensity > MIN_ACTIVE_PIXELS || right_movement_intensity > MIN_ACTIVE_PIXELS) {
                printf("Snake8Bit movement detected: left=%d, right=%d\n", left_movement_intensity, right_movement_intensity);
                write_movement_to_pipe(pipe_path);
            }
        }

        // Update the previous frame
        prev_frame = frame.clone();

        // Adjust direction based on movement intensity
        if (right_movement_intensity > left_movement_intensity) {
            // Turn left
            float angle = -CV_PI / 18;  // Turn angle in radians
            float new_vx = snake.velocity.x * cos(angle) - snake.velocity.y * sin(angle);
            float new_vy = snake.velocity.x * sin(angle) + snake.velocity.y * cos(angle);
            snake.velocity.x = new_vx;
            snake.velocity.y = new_vy;
        }
        if (left_movement_intensity > right_movement_intensity) {
            // Turn right
            float angle = CV_PI / 18;  // Turn angle in radians
            float new_vx = snake.velocity.x * cos(angle) - snake.velocity.y * sin(angle);
            float new_vy = snake.velocity.x * sin(angle) + snake.velocity.y * cos(angle);
            snake.velocity.x = new_vx;
            snake.velocity.y = new_vy;
        }

        // Update position
        Point2f new_head_position = snake.body[0] + snake.velocity;
        
        // Check for collisions with the edges of the display
        if (new_head_position.x - snake.radius < 0) {
            new_head_position.x = snake.radius;
            snake.velocity.x = abs(snake.velocity.x); // Bounce right
        } else if (new_head_position.x + snake.radius > DISPLAY_WIDTH) {
            new_head_position.x = DISPLAY_WIDTH - snake.radius;
            snake.velocity.x = -abs(snake.velocity.x); // Bounce left
        }
        if (new_head_position.y - snake.radius < 0) {
            new_head_position.y = snake.radius;
            snake.velocity.y = abs(snake.velocity.y); // Bounce down
        } else if (new_head_position.y + snake.radius > DISPLAY_HEIGHT) {
            new_head_position.y = DISPLAY_HEIGHT - snake.radius;
            snake.velocity.y = -abs(snake.velocity.y); // Bounce up
        }

        // Add new head position
        snake.body.insert(snake.body.begin(), new_head_position);

        // Check for food collisions
        for (auto it = food_particles.begin(); it != food_particles.end(); ) {
            if (norm(snake.body[0] - it->position) < (snake.radius + it->radius)) {
                // Eat the food particle
                it = food_particles.erase(it);
                // Add a segment to the snake
                snake.segments_to_add += 1;
            } else {
                ++it;
            }
        }

        // Remove the last segment if no new segments are being added
        if (snake.segments_to_add == 0) {
            snake.body.pop_back();
        } else {
            snake.segments_to_add--;
        }

        // Clear the LED wall
        led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);

        // Draw the grid
        for (int y = 0; y < DISPLAY_HEIGHT; y += GRID_SIZE) {
            line(led_wall, Point(0, y), Point(DISPLAY_WIDTH, y), Scalar(50, 50, 50));
        }
        for (int x = 0; x < DISPLAY_WIDTH; x += GRID_SIZE) {
            line(led_wall, Point(x, 0), Point(x, DISPLAY_HEIGHT), Scalar(50, 50, 50));
        }

        // Draw the snake
        for (const auto &segment : snake.body) {
            cv::circle(led_wall, segment, snake.radius, snake.color, FILLED);
        }

        // Draw the food particles
        for (const auto &food : food_particles) {
            cv::circle(led_wall, food.position, food.radius, food.color, FILLED);
        }

        // Display the LED wall simulation
        imshow("Snake8Bit", led_wall);

        // Exit the loop on 'q' key press
        if (waitKey(30) == 'q') break;
    }

    // Release the camera
    cap.release();
    destroyAllWindows();
}

int main(int argc, char** argv) {
    const char *pipe_path = "/tmp/movement_pipe";

    // Create the named pipe
    create_pipe(pipe_path);

    int mode = mode_selector(pipe_path);
    if (mode == 1) {
        brickPong(pipe_path);
    } else if (mode == 2) {
        snake8Bit(pipe_path);
    } else {
        printf("Error: No valid mode selected\n");
    }

    return 0;
}
