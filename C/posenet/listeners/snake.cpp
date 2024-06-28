#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <iostream>

#define LED_WIDTH 20
#define LED_HEIGHT 20
#define DISPLAY_WIDTH 640
#define DISPLAY_HEIGHT 480
#define LED_SPACING 5
#define NUM_FOOD_PARTICLES 10  // Number of food particles
#define GRID_SIZE 20  // Size of each grid cell
#define FRAME_RATE 10  // Frame rate to reduce processing load

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
    snake.velocity = Point2f(0, -3);  // Initial velocity upwards
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

void update_snake(Snake &snake, float hand_x, float hand_y, bool hand_detected, vector<Particle> &food_particles) {
    if (hand_detected) {
        // Flip hand_x by mirroring it around the center of the display width
        float inverted_hand_x = DISPLAY_WIDTH - hand_x;

        Point2f center(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
        Point2f hand_position(inverted_hand_x, hand_y);
        Point2f direction = hand_position - center;
        float angle = atan2(direction.y, direction.x);
        snake.velocity.x = cos(angle) * 3;  // Speed of 3
        snake.velocity.y = sin(angle) * 3;
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
    led_wall.setTo(Scalar(0, 0, 0));

    // Draw the grid
    for (int y = 0; y < DISPLAY_HEIGHT; y += GRID_SIZE) {
        line(led_wall, Point(0, y), Point(DISPLAY_WIDTH, y), Scalar(50, 50, 50));
    }
    for (int x = 0; x < DISPLAY_WIDTH; x += GRID_SIZE) {
        line(led_wall, Point(x, 0), Point(x, DISPLAY_HEIGHT), Scalar(50, 50, 50));
    }

    // Draw the snake
    for (const auto &segment : snake.body) {
        circle(led_wall, segment, snake.radius, snake.color, FILLED);
    }

    // Draw the food particles
    for (const auto &food : food_particles) {
        circle(led_wall, food.position, food.radius, food.color, FILLED);
    }
}

int main(int argc, char** argv) {
    // Open the named pipe for reading
    const char* pipePath = "/tmp/movement_pipe";
    int pipe_fd = open(pipePath, O_RDONLY | O_NONBLOCK); // Open pipe in non-blocking mode
    if (pipe_fd == -1) {
        printf("Error: Could not open named pipe\n");
        return -1;
    }

    Mat frame;
    Snake snake;
    vector<Particle> food_particles;
    float hand_x = DISPLAY_WIDTH / 2;  // Initialize hand_x at the center
    float hand_y = DISPLAY_HEIGHT / 2;  // Initialize hand_y at the center
    bool hand_detected = false;  // Flag to indicate if a hand is detected

    // Initialize the LED wall, snake, and food particles
    Mat led_wall(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);
    initialize_led_wall(led_wall);
    initialize_snake(snake);
    initialize_food(food_particles);

    // Set a lower frame rate to reduce processing load
    int frame_delay = 1000 / FRAME_RATE;

    while (true) {
        // Read from the named pipe
        char buffer[256];
        ssize_t bytesRead = read(pipe_fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::istringstream iss(buffer);
            std::string line;
            while (std::getline(iss, line)) {
                // Parse the coordinates from the line
                if (sscanf(line.c_str(), "Pose %*d, Keypoint %*d: (%f, %f)", &hand_x, &hand_y) == 2) {
                    // Hand detected
                    hand_detected = true;
                    // Debugging: Print the hand coordinates
                    cout << "Hand coordinates: (" << hand_x << ", " << hand_y << ")" << endl;
                }
            }
        } else {
            // No hand detected
            hand_detected = false;
        }

        // Update the snake based on hand coordinates and check for food collisions
        update_snake(snake, hand_x, hand_y, hand_detected, food_particles);

        // Draw the LED wall
        draw_led_wall(led_wall, snake, food_particles);

        // Display the LED wall simulation
        imshow("LED PCB Wall Simulation", led_wall);

        // Exit the loop on 'q' key press
        if (waitKey(frame_delay) == 'q') break;
    }

    // Close the named pipe
    close(pipe_fd);

    // Release the camera
    destroyAllWindows();

    return 0;
}
