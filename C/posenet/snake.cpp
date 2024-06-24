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

#define DISPLAY_WIDTH 640
#define DISPLAY_HEIGHT 480
#define GRID_SIZE 20  // Size of each grid cell
#define TURN_ANGLE CV_PI / 18  // Turn angle in radians
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

void initialize_snake(Snake &snake) {
    snake.body.push_back(Point2f(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2));
    snake.velocity = Point2f(3, 0);  // Initial velocity to the right
    snake.radius = 10;
    snake.color = Scalar(0, 0, 255);  // Red color
    snake.segments_to_add = 0;  // Initialize with no segments to add
}

void initialize_food(vector<Particle> &food_particles) {
    srand(time(0));
    for (int i = 0; i < 10; ++i) {
        Particle food;
        food.position = Point2f(rand() % DISPLAY_WIDTH, rand() % DISPLAY_HEIGHT);
        food.radius = 5;
        food.color = Scalar(0, 255, 0);  // Green color
        food_particles.push_back(food);
    }
}

void update_snake(Snake &snake, float hand_x, bool hand_detected) {
    if (hand_detected) {
        if (hand_x > DISPLAY_WIDTH / 2) {
            // Turn right
            float new_vx = snake.velocity.x * cos(TURN_ANGLE) - snake.velocity.y * sin(TURN_ANGLE);
            float new_vy = snake.velocity.x * sin(TURN_ANGLE) + snake.velocity.y * cos(TURN_ANGLE);
            snake.velocity.x = new_vx;
            snake.velocity.y = new_vy;
        } else {
            // Turn left
            float new_vx = snake.velocity.x * cos(-TURN_ANGLE) - snake.velocity.y * sin(-TURN_ANGLE);
            float new_vy = snake.velocity.x * sin(-TURN_ANGLE) + snake.velocity.y * cos(-TURN_ANGLE);
            snake.velocity.x = new_vx;
            snake.velocity.y = new_vy;
        }
    }

    // Update position
    Point2f new_head_position = snake.body[0] + snake.velocity;

    // Check for collisions with the edges of the display
    if (new_head_position.x - snake.radius < 0 || new_head_position.x + snake.radius > DISPLAY_WIDTH) {
        snake.velocity.x = -snake.velocity.x; // Bounce
    }
    if (new_head_position.y - snake.radius < 0 || new_head_position.y + snake.radius > DISPLAY_HEIGHT) {
        snake.velocity.y = -snake.velocity.y; // Bounce
    }

    new_head_position = snake.body[0] + snake.velocity;  // Recalculate after potential bounce
    snake.body.insert(snake.body.begin(), new_head_position);

    // Remove the last segment if no new segments are being added
    if (snake.segments_to_add == 0) {
        snake.body.pop_back();
    } else {
        snake.segments_to_add--;
    }
}

void draw_game(Mat &frame, const Snake &snake, const vector<Particle> &food_particles) {
    frame.setTo(Scalar(0, 0, 0));

    // Draw the snake
    for (const auto &segment : snake.body) {
        circle(frame, segment, snake.radius, snake.color, FILLED);
    }

    // Draw the food particles
    for (const auto &food : food_particles) {
        circle(frame, food.position, food.radius, food.color, FILLED);
    }
}

int main(int argc, char** argv) {
    const char* pipePath = "/tmp/movement_pipe";
    int pipe_fd = open(pipePath, O_RDONLY | O_NONBLOCK);
    if (pipe_fd == -1) {
        printf("Error: Could not open named pipe\n");
        return -1;
    }

    Snake snake;
    vector<Particle> food_particles;
    float hand_x = DISPLAY_WIDTH / 2;
    bool hand_detected = false;

    Mat frame(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3);
    initialize_snake(snake);
    initialize_food(food_particles);

    int frame_delay = 1000 / FRAME_RATE;

    while (true) {
        // Read from the named pipe
        char buffer[256];
        ssize_t bytesRead = read(pipe_fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            float hand_y;
            std::istringstream iss(buffer);
            std::string line;
            while (std::getline(iss, line)) {
                if (sscanf(line.c_str(), "Pose %*d, Keypoint %*d: (%f, %f)", &hand_x, &hand_y) == 2) {
                    hand_detected = true;
                }
            }
        } else {
            hand_detected = false;
        }

        // Update the snake based on hand_x and check for food collisions
        update_snake(snake, hand_x, hand_detected);

        // Draw the game frame
        draw_game(frame, snake, food_particles);

        // Display the game
        imshow("Snake Game", frame);

        // Exit the loop on 'q' key press
        if (waitKey(frame_delay) == 'q') break;
    }

    close(pipe_fd);
    destroyAllWindows();

    return 0;
}
