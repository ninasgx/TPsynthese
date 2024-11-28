#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>  // For clock_gettime

#define MAX_INPUT_LENGTH 1024

int main() {
    char command[MAX_INPUT_LENGTH];
    pid_t pid;  // Child ID
    int status;  // Child exit status
    struct timespec start, end; 
    long elapsed_ms;

    const char *welcome_message = "Welcome to ENSEA (Alamo and Sabrina) Tiny Shell.\n";
    write(STDOUT_FILENO, welcome_message, strlen(welcome_message));

    const char *exit_message = "Type 'exit' to quit or <ctrl>+d.\n";
    write(STDOUT_FILENO, exit_message, strlen(exit_message));

    while (1) {
        write(STDOUT_FILENO, "enseash% ", 8);

        if (fgets(command, MAX_INPUT_LENGTH, stdin) == NULL) {
            if (feof(stdin)) {  // Check EOF / Ctrl+D
                write(STDOUT_FILENO, "Bye bye...\n", 11);
                exit(0); 
            } else {
                write(STDOUT_FILENO, "Error reading command.\n", 23);
                continue;
            }
        }

        command[strcspn(command, "\n")] = 0;

        if (strcmp(command, "exit") == 0) {
            write(STDOUT_FILENO, "Bye bye...\n", 11);
            exit(0); 
        }

        clock_gettime(CLOCK_MONOTONIC, &start);  // Start time here

        pid = fork(); // Create a child process to execute the command
        
        if (pid == -1) {
            write(STDOUT_FILENO, "Fork failed.\n", 13);
            continue;
        }

        if (pid == 0) {
            char *args[] = {command, NULL}; 
            if (execvp(command, args) == -1) {
                write(STDOUT_FILENO, "Command execution failed.\n", 25);
                exit(1);
            }
        } else {
            waitpid(pid, &status, 0); // Parent waits for the child process to finish
        }

        clock_gettime(CLOCK_MONOTONIC, &end); // End time here

       // Display the exit code or signal of the last executed command
        if (WIFEXITED(status)) { // If it exited normally, show exit code
            char exit_msg[50];
            snprintf(exit_msg, sizeof(exit_msg), "[exit:%d", WEXITSTATUS(status));
            write(STDOUT_FILENO, exit_msg, strlen(exit_msg));
        } else if (WIFSIGNALED(status)) {  // If it was terminated by a signal, show the signal number
            char sign_msg[50];
            snprintf(sign_msg, sizeof(sign_msg), "[sign:%d", WTERMSIG(status));
            write(STDOUT_FILENO, sign_msg, strlen(sign_msg));
        }

        clock_gettime(CLOCK_MONOTONIC, &end); // End time here

        elapsed_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000; // Calculate elapsed time in milliseconds

        char time_msg[50];
        snprintf(time_msg, sizeof(time_msg), "|%ldms] ", elapsed_ms);
        write(STDOUT_FILENO, time_msg, strlen(time_msg));

        write(STDOUT_FILENO, " ", 2);
    }

    return 0;
}
