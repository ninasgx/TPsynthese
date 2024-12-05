#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_INPUT_LENGTH 1024
#define MAX_ARGS 64

const int NANOSECONDS_IN_MILLISECOND = 1000000; 
const int MILLISECONDS_IN_SECOND = 1000;

void display_welcome_message() {
    const char *welcome_message = "Welcome to ENSEA (Alamo and Sabrina) Tiny Shell.\n";
    write(STDOUT_FILENO, welcome_message, strlen(welcome_message));
    const char *exit_message = "Type 'exit' to quit or <ctrl>+d.\n";
    write(STDOUT_FILENO, exit_message, strlen(exit_message));
}

void display_prompt() {
    write(STDOUT_FILENO, "enseash% ", 9);
}

void display_exit_message() {
    write(STDOUT_FILENO, "Bye bye...\n", 11);
}

char *read_command() {
    static char command[MAX_INPUT_LENGTH];
    ssize_t bytes_read = read(STDIN_FILENO, command, MAX_INPUT_LENGTH - 1);
    if (bytes_read <= 0) {
        if (bytes_read == 0) { 
            display_exit_message();
            exit(0);
        } else {
            write(STDOUT_FILENO, "Error reading command.\n", 23);
            return NULL;
        }
    }
    command[bytes_read - 1] = '\0'; 
    return command;
}

int is_exit_command(const char *command) {
    return strcmp(command, "exit") == 0;
}

void parse_command(const char *input, char **args, char **input_file, char **output_file) {
    *input_file = NULL;
    *output_file = NULL;
    char *token = strtok((char *)input, " ");
    int i = 0;
    while (token != NULL && i < MAX_ARGS - 1) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            *input_file = token;
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            *output_file = token; 
        } else {
            args[i++] = token;
        }
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
}

void execute_command(char *command) {
    char *args[MAX_ARGS];
    char *input_file = NULL;
    char *output_file = NULL;

    parse_command(command, args, &input_file, &output_file);

    if (input_file) {
        int fd_in = open(input_file, O_RDONLY);
        if (fd_in == -1) {
            perror("open input file");
            exit(1);
        }
        dup2(fd_in, STDIN_FILENO); 
        close(fd_in);
    }

    if (output_file) {
        int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out == -1) {
            perror("open output file");
            exit(1);
        }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }

    if (execvp(args[0], args) == -1) {
        perror("Command execution failed");
        exit(1);
    }
}

void run_shell() {
    char *command;
    pid_t pid;
    int status;
    struct timespec start, end;
    long elapsed_ms;

    while (1) {
        display_prompt();
        command = read_command();
        if (command == NULL) continue;

        if (is_exit_command(command)) {
            display_exit_message();
            exit(0);
        }

        clock_gettime(CLOCK_MONOTONIC, &start);

        pid = fork();

        if (pid == -1) {
            write(STDOUT_FILENO, "Fork failed.\n", 13);
            continue;
        }

        if (pid == 0) {
            execute_command(command);
        } else {
            waitpid(pid, &status, 0);
            clock_gettime(CLOCK_MONOTONIC, &end);

            elapsed_ms = (end.tv_sec - start.tv_sec) * MILLISECONDS_IN_SECOND +
                         (end.tv_nsec - start.tv_nsec) / NANOSECONDS_IN_MILLISECOND;

            if (WIFEXITED(status)) {
                dprintf(STDOUT_FILENO, "[exit:%d|%ldms] ", WEXITSTATUS(status), elapsed_ms);
            } else if (WIFSIGNALED(status)) {
                dprintf(STDOUT_FILENO, "[sign:%d|%ldms] ", WTERMSIG(status), elapsed_ms);
            }
        }
    }
}

int main() {
    display_welcome_message();
    run_shell();
    return 0;
}