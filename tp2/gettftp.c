#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#define BUFFER_SIZE 1024

void write_message(int fd, const char *message) {
    write(fd, message, strlen(message));
}

void read_input(int fd, char *buffer, size_t size) {
    read(fd, buffer, size);
    buffer[strcspn(buffer, "\n")] = 0;
}

int resolve_address(const char *server_address, struct addrinfo **result) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      // IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM;   // Use UDP

    int s = getaddrinfo(server_address, "69", &hints, result);
    if (s != 0) {
        char error_msg[BUFFER_SIZE];
        snprintf(error_msg, BUFFER_SIZE, "getaddrinfo: %s\n", gai_strerror(s));
        write_message(STDERR_FILENO, error_msg);
        return -1;
    }
    return 0;
}

int connect_to_server(struct addrinfo *result) {
    struct addrinfo *p;
    for (p = result; p != NULL; p = p->ai_next) {
        int sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            continue; 
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }

        return sockfd;
    }
    
    return -1;
}

// Main function
int main(int argc, char *argv[]) {
    if (argc != 2) {
        write_message(STDERR_FILENO, "Usage: ./gettftp <server_address>\n");
        exit(EXIT_FAILURE);
    }

    struct addrinfo *result;
    
    if (resolve_address(argv[1], &result) != 0) {
        exit(EXIT_FAILURE);
    }

    int sockfd = connect_to_server(result);
    
    if (sockfd == -1) {
        write_message(STDERR_FILENO, "Failed to connect to any address\n");
        freeaddrinfo(result); 
        exit(EXIT_FAILURE);
    }

    write_message(STDOUT_FILENO, "Successfully connected to the server\n");

    freeaddrinfo(result);

    close(sockfd);
    return 0;
}