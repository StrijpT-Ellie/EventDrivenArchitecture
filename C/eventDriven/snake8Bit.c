#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

#define LED_WIDTH 20
#define LED_HEIGHT 20
#define DISPLAY_WIDTH 640
#define DISPLAY_HEIGHT 480
#define LED_SPACING 5
#define MOVEMENT_THRESHOLD 30  // Threshold to detect movement
#define NUM_FOOD_PARTICLES 10  // Number of food particles
#define GRID_SIZE 20  // Size of each grid cell

using namespace cv;
using namespace std;

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

void initialize_led_wall(Mat &led_wall) {
    led_wall = Mat::zeros(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);
}

void initialize_snake(Snake &snake) {
    snake.body.push_back(Point2f(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2));
    snake.velocity = Point2f(3, 0);  // Initial velocity to the right
    snake.radius = 10;
    snake.color = Scalar(0, 0, 255);  // Red color
    snake.segments_to_add = 0;  // Initialize with no segments to add
}

void initialize_food(vector<Particle> &food_particles) {
    srand(time(0));
    for (int i = 0; i < NUM_FOOD_PARTICLES; ++i) {
        Particle food;
        food.position = Point2f(rand() % DISPLAY_WIDTH, rand() % DISPLAY_HEIGHT);
        food.radius = 5;
        food.color = Scalar(0, 255, 0);  // Green color
        food_particles.push_back(food);
    }
}

void detect_movement(const Mat &prev_frame, const Mat &current_frame, int &left_movement_intensity, int &right_movement_intensity) {
    Mat gray_prev, gray_current, diff;
    cvtColor(prev_frame, gray_prev, COLOR_BGR2GRAY);
    cvtColor(current_frame, gray_current, COLOR_BGR2GRAY);
    absdiff(gray_prev, gray_current, diff);
    threshold(diff, diff, MOVEMENT_THRESHOLD, 255, THRESH_BINARY);

    Mat left_half = diff(Rect(0, 0, diff.cols / 2, diff.rows));
    Mat right_half = diff(Rect(diff.cols / 2, 0, diff.cols / 2, diff.rows));

    left_movement_intensity = countNonZero(left_half);
    right_movement_intensity = countNonZero(right_half);
}

void update_snake(Snake &snake, int left_movement_intensity, int right_movement_intensity, vector<Particle> &food_particles) {
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
}

void draw_led_wall(Mat &led_wall, const Snake &snake, const vector<Particle> &food_particles) {
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

    Mat frame, prev_frame;
    Snake snake;
    vector<Particle> food_particles;
    int left_movement_intensity = 0;
    int right_movement_intensity = 0;

    // Initialize the LED wall, snake, and food particles
    Mat led_wall;
    initialize_led_wall(led_wall);
    initialize_snake(snake);
    initialize_food(food_particles);

    while (true) {
        // Capture a new frame
        cap >> frame;
        if (frame.empty()) {
            printf("Error: No captured frame\n");
            break;
        }

        // Flip the frame horizontally to correct the mirrored view
        // flip(frame, frame, 1);

        // Resize the frame to match the LED PCB wall resolution
        resize(frame, frame, Size(DISPLAY_WIDTH, DISPLAY_HEIGHT), 0, 0, INTER_LINEAR);

        // If there's a previous frame, detect movement
        if (!prev_frame.empty()) {
            detect_movement(prev_frame, frame, left_movement_intensity, right_movement_intensity);
        }

        // Update the previous frame
        prev_frame = frame.clone();

        // Update the snake based on movement intensity and check for food collisions
        update_snake(snake, left_movement_intensity, right_movement_intensity, food_particles);

        // Draw the LED wall
        draw_led_wall(led_wall, snake, food_particles);

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
