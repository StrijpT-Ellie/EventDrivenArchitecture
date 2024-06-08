#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#define TIMEOUT 15
#define PIPE_BUF 1024

pid_t launch_script(const char *script_path) {
    pid_t pid = fork();
    if (pid == 0) {
        execl(script_path, script_path, NULL);
        perror("execl failed");
        exit(EXIT_FAILURE);
    }
    return pid;
}

void kill_script(pid_t pid) {
    kill(pid, SIGTERM);
    waitpid(pid, NULL, 0);
}

int check_movement(const char *pipe_path) {
    int fd = open(pipe_path, O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        perror("open pipe failed");
        return 0;
    }

    char buffer[PIPE_BUF];
    ssize_t n = read(fd, buffer, sizeof(buffer));
    close(fd);

    if (n > 0) {
        printf("Movement detected in master script: %.*s\n", (int)n, buffer);
        return 1;
    } else if (n == -1 && errno != EAGAIN) {
        perror("read pipe failed");
    }
    return 0;
}

int main() {
    const char *scripts[] = { "./arrayNoVideo", "./fadingPixels" };
    const char *pipe_path = "/tmp/movement_pipe";

    if (mkfifo(pipe_path, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo failed");
            return 1;
        }
    }

    size_t current_script = 0;
    pid_t current_pid = launch_script(scripts[current_script]);

    time_t last_movement = time(NULL);

    while (1) {
        sleep(1);

        int movement_detected = check_movement(pipe_path);

        if (movement_detected) {
            last_movement = time(NULL);
        }

        time_t now = time(NULL);
        printf("Time since last movement: %.0f seconds\n", difftime(now, last_movement));

        if (difftime(now, last_movement) > TIMEOUT) {
            printf("Timeout reached, switching scripts\n");
            kill_script(current_pid);
            current_script = (current_script + 1) % 2;
            current_pid = launch_script(scripts[current_script]);
            last_movement = time(NULL);
        }

        if (current_script == 1 && difftime(now, last_movement) > TIMEOUT) {
            printf("Switching back to script 1\n");
            kill_script(current_pid);
            current_script = 0;
            current_pid = launch_script(scripts[current_script]);
            last_movement = time(NULL);
        }
    }

    return 0;
}
