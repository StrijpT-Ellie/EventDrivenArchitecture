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

#define TIMEOUT 15

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

    char buffer[1024];
    ssize_t n = read(fd, buffer, sizeof(buffer));
    close(fd);
    return n > 0;
}

int main() {
    const char *scripts[] = { "./arrayNoVideo", "./fadingPixels" };
    const char *pipe_path = "/tmp/movement_pipe";

    mkfifo(pipe_path, 0666);

    size_t current_script = 0;
    pid_t current_pid = launch_script(scripts[current_script]);

    time_t last_movement = time(NULL);

    while (1) {
        sleep(1);

        if (check_movement(pipe_path)) {
            last_movement = time(NULL);
        }

        if (difftime(time(NULL), last_movement) > TIMEOUT) {
            kill_script(current_pid);
            current_script = (current_script + 1) % 2;
            current_pid = launch_script(scripts[current_script]);
            last_movement = time(NULL);
        }

        // Ensure it returns to script1 after script2 times out
        if (current_script == 1 && difftime(time(NULL), last_movement) > TIMEOUT) {
            kill_script(current_pid);
            current_script = 0;
            current_pid = launch_script(scripts[current_script]);
            last_movement = time(NULL);
        }
    }

    return 0;
}
