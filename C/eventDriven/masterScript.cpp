#include <stdio.h>      // Standard input/output library
#include <stdlib.h>     // Standard library for general functions
#include <unistd.h>     // Unix standard functions for system calls
#include <sys/types.h>  // Data types used in system calls
#include <sys/wait.h>   // Macros related to process termination
#include <signal.h>     // Signal handling functions and macros
#include <time.h>       // Time library for handling timestamps
#include <fcntl.h>      // File control options
#include <string.h>     // String handling functions
#include <sys/stat.h>   // Functions for file status
#include <errno.h>      // Error number definitions
#include <poll.h>       // Polling functions

#define TIMEOUT 15      // Timeout period in seconds for switching scripts
#define PIPE_BUF 1024   // Buffer size for reading from the pipe

// Function to launch a script and return its process ID
pid_t launch_script(const char *script_path) {
    pid_t pid = fork(); // Create a new process
    if (pid == 0) {     // Child process block
        execl(script_path, script_path, NULL); // Replace child process with the script
        perror("execl failed"); // Print error message if execl fails
        exit(EXIT_FAILURE); // Exit child process with failure code
    }
    return pid; // Return the process ID of the child process
}

// Function to kill a script given its process ID
void kill_script(pid_t pid) {
    kill(pid, SIGTERM); // Send SIGTERM signal to terminate the process
    waitpid(pid, NULL, 0); // Wait for the process to terminate and clean up
}

// Function to check for movement by reading from the named pipe
int check_movement(const char *pipe_path) {
    int fd = open(pipe_path, O_RDONLY | O_NONBLOCK); // Open the pipe for reading in non-blocking mode
    if (fd == -1) { // Check if the pipe failed to open
        perror("open pipe failed"); // Print error message
        return 0; // Return 0 indicating no movement detected
    }

    struct pollfd pfd = { .fd = fd, .events = POLLIN };
    int poll_result = poll(&pfd, 1, 1000); // Poll with a timeout of 1 second

    if (poll_result == -1) {
        perror("poll failed");
        close(fd);
        return 0;
    } else if (poll_result == 0) {
        printf("No data in pipe\n");
        close(fd);
        return 0;
    }

    char buffer[PIPE_BUF]; // Buffer to store data read from the pipe
    ssize_t n = read(fd, buffer, sizeof(buffer) - 1); // Read data from the pipe
    close(fd); // Close the pipe

    if (n > 0) { // If data was read from the pipe
        buffer[n] = '\0'; // Null-terminate the string read from the pipe
        printf("Movement detected in master script: %s\n", buffer); // Print the detected movement
        return 1; // Return 1 indicating movement detected
    } else if (n == -1 && errno != EAGAIN) { // Check for read errors other than no data (EAGAIN)
        perror("read pipe failed"); // Print error message
    }

    return 0; // Return 0 indicating no movement detected
}

int main() {
    const char *scripts[] = { "./arrayNoVideo", "./fadingPixels" }; // Array of script paths
    const char *pipe_path = "/tmp/movement_pipe"; // Path to the named pipe

    // Remove any existing pipe if it exists and create a new one
    unlink(pipe_path); // Remove the existing named pipe, if any
    if (mkfifo(pipe_path, 0666) == -1) { // Create a new named pipe with read/write permissions
        perror("mkfifo failed"); // Print error message if pipe creation fails
        exit(EXIT_FAILURE); // Exit the program with failure code
    }

    size_t current_script = 0; // Index of the currently running script
    pid_t current_pid = launch_script(scripts[current_script]); // Launch the initial script and get its process ID

    time_t last_movement = time(NULL); // Record the current time as the last movement time

    while (1) {
        int movement_detected = check_movement(pipe_path); // Check for movement by reading from the pipe

        if (movement_detected) { // If movement was detected
            last_movement = time(NULL); // Update the last movement time to the current time
            printf("Resetting last_movement to current time: %ld\n", last_movement);
        }

        time_t now = time(NULL); // Get the current time
        double time_since_last_movement = difftime(now, last_movement); // Calculate time since last movement
        printf("Time since last movement: %.0f seconds\n", time_since_last_movement); // Print the time since the last movement

        if (time_since_last_movement > TIMEOUT) { // If the timeout period has elapsed since the last movement
            printf("Timeout reached, switching scripts...\n");
            kill_script(current_pid); // Kill the currently running script
            current_script = (current_script + 1) % 2; // Switch to the other script
            current_pid = launch_script(scripts[current_script]); // Launch the new script and get its process ID
            last_movement = time(NULL); // Reset the last movement time to the current time
        }

        sleep(1); // Sleep for 1 second between checks
    }

    return 0; // Exit the program
}
