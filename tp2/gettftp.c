#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        char error_msg[] = "Usage: ";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        write(STDERR_FILENO, argv[0], strlen(argv[0]));
        char usage[] = " <host> <file>\n";
        write(STDERR_FILENO, usage, strlen(usage));
        exit(1);
    }

    char *host = argv[1];
    char *file = argv[2];

    char buffer[BUFFER_SIZE];
    int len;

    len = snprintf(buffer, BUFFER_SIZE, "Host: %s\n", host);
    write(STDOUT_FILENO, buffer, len);

    len = snprintf(buffer, BUFFER_SIZE, "File: %s\n", file);
    write(STDOUT_FILENO, buffer, len);

    return 0;
}