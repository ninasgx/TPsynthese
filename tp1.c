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
#define MAX_BG_PROCESSES 100

const int NANOSECONDS_IN_MILLISECOND = 1000000; 
const int MILLISECONDS_IN_SECOND = 1000;

typedef struct { //For background processes
    pid_t pid;               
    int job_id;             
    char command[MAX_INPUT_LENGTH]; 
} BackgroundProcess;

BackgroundProcess bg_processes[MAX_BG_PROCESSES]; // Array to hold 
int bg_count = 0; 

void display_welcome_message();
void display_prompt();
void display_exit_message();
char *read_command();
int is_exit_command(const char *command);
void parse_command(const char *input, char **args, char **input_file, char **output_file, char **pipe_cmd);
void execute_command(char *command);
void execute_pipe(char *cmd1, char *cmd2);
void check_background_processes();

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
    const char *exit_message = "Bye bye...\n";
    write(STDOUT_FILENO, exit_message, strlen(exit_message));
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

void parse_command(const char *input, char **args, char **input_file, char **output_file, char **pipe_cmd) {
    *input_file = NULL;
    *output_file = NULL;
    *pipe_cmd = NULL;
    char *token = strtok((char *)input, " ");
    int i = 0;
    
    while (token != NULL && i < MAX_ARGS - 1) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            *input_file = token;
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            *output_file = token;
        } else if (strcmp(token, "|") == 0) {
            token = strtok(NULL, " ");
            *pipe_cmd = token;
            break;
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
    char *pipe_cmd = NULL;

    parse_command(command, args, &input_file, &output_file, &pipe_cmd);

    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    int background = 0;

    int arg_count = 0;
    while (args[arg_count] != NULL) {
        arg_count++;
    }

    if (arg_count > 0 && strcmp(args[arg_count - 1], "&") == 0) {
        args[arg_count - 1] = NULL; 
        background = 1;  // 1 sets a flag for background execution
    }

    pid_t pid = fork();
    
    if (pid == 0) { 
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

        execvp(args[0], args);
        perror("execvp");
        exit(1);
    } else { 
        if (background) {
            bg_processes[bg_count].pid = pid;
            bg_processes[bg_count].job_id = bg_count + 1; 
            strcpy(bg_processes[bg_count].command, command);
            bg_count++;
            dprintf(STDOUT_FILENO, "[%d] %d\n", bg_processes[bg_count - 1].job_id, pid);
        } else {
            waitpid(pid, NULL, 0);
        }
    }
}

void check_background_processes() {
    for (int i = 0; i < bg_count; i++) {
        int status;
        pid_t result = waitpid(bg_processes[i].pid, &status, WNOHANG);
        
        if (result == 0) {
            continue;
        } else if (result == -1) {
            perror("waitpid");
        } else {
            if (WIFEXITED(status)) {
                dprintf(STDOUT_FILENO, "[%d] %s: exited with status %d\n", bg_processes[i].job_id, bg_processes[i].command, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                dprintf(STDOUT_FILENO, "[%d] %s: killed by signal %d\n", bg_processes[i].job_id, bg_processes[i].command, WTERMSIG(status));
            }
            bg_processes[i] = bg_processes[--bg_count]; 
            i--; 
        }
    }
}

void run_shell() {
    char *command;
    struct timespec start, end;
    
    while (1) {
        display_prompt();
        command = read_command();
        
        if (command == NULL) continue;

        clock_gettime(CLOCK_MONOTONIC, &start);

        execute_command(command);

        check_background_processes(); 
        
        clock_gettime(CLOCK_MONOTONIC, &end); 
        
        long elapsed_ms = (end.tv_sec - start.tv_sec) * MILLISECONDS_IN_SECOND +
                          (end.tv_nsec - start.tv_nsec) / NANOSECONDS_IN_MILLISECOND;

        dprintf(STDOUT_FILENO, "[exit: %ldms] \n", elapsed_ms); 
    }
}

int main() {
    display_welcome_message();
    run_shell();
    return 0;
}