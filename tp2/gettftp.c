#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 516
#define DATA_SIZE 512
#define TFTP_OPCODE_SIZE 2
#define TFTP_NULL_TERMINATOR 1
// RRQ/WRQ format: [2 bytes opcode][filename + '\0'][mode + '\0']

void write_message(int fd, const char *message) {
    write(fd, message, strlen(message));
}

int resolve_address(const char *server_address, struct addrinfo **result) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      
    hints.ai_socktype = SOCK_DGRAM;   

    int status = getaddrinfo(server_address, "1069", &hints, result);
    if (status != 0) {
        char error_msg[BUFFER_SIZE];
        snprintf(error_msg, BUFFER_SIZE, "getaddrinfo: %s\n", gai_strerror(status));
        write_message(STDERR_FILENO, error_msg);
        return -1;
    }
    return 0;
}

void build_rrq(char *buffer, const char *filename, const char *mode) {
    uint16_t opcode = htons(1); 
    size_t offset = 0;

    memcpy(buffer + offset, &opcode, TFTP_OPCODE_SIZE);
    offset += TFTP_OPCODE_SIZE;
    strcpy(buffer + offset, filename);
    offset += strlen(filename) + TFTP_NULL_TERMINATOR;
    strcpy(buffer + offset, mode);
    offset += strlen(mode) + TFTP_NULL_TERMINATOR;
    // RRQ packet built: opcode + filename+'\0' + mode+'\0'
}

void send_rrq_and_receive(const char *server, const char *filename) {
    struct addrinfo *res;
    char buffer[BUFFER_SIZE];
    int file_fd;
    ssize_t received_bytes, sent_bytes;
    uint16_t current_block = 0;

    if (resolve_address(server, &res) != 0) {
        exit(EXIT_FAILURE);
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == -1) {
        write_message(STDOUT_FILENO, "Error creating socket\n");
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    build_rrq(buffer, filename, "octet");

    // Total length: opcode(2 bytes) + filename+'\0' + mode+'\0'
    size_t rrq_length = TFTP_OPCODE_SIZE 
                        + (strlen(filename) + TFTP_NULL_TERMINATOR) 
                        + (strlen("octet") + TFTP_NULL_TERMINATOR);

    sent_bytes = sendto(sock, buffer, rrq_length, 0, res->ai_addr, res->ai_addrlen);
    if (sent_bytes == -1) {
        write_message(STDOUT_FILENO, "Error sending RRQ\n");
        close(sock);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    write_message(STDOUT_FILENO, "RRQ sent to server\n");

    file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_fd == -1) {
        write_message(STDOUT_FILENO, "Error creating local file\n");
        close(sock);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    while (1) {
        received_bytes = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&sender_addr, &sender_len);
        if (received_bytes == -1) {
            write_message(STDOUT_FILENO, "Error receiving data\n");
            break;
        }

        uint16_t opcode, block_num;
        memcpy(&opcode, buffer, sizeof(uint16_t));
        opcode = ntohs(opcode);

        if (opcode == 3) {  
            memcpy(&block_num, buffer + 2, sizeof(uint16_t));
            block_num = ntohs(block_num);

            if (block_num == current_block + 1) {
                current_block++;
                char msg[128];
                snprintf(msg, sizeof(msg), "Received DATA block %d, size %ld bytes\n", current_block, (long)(received_bytes - 4));
                write(STDOUT_FILENO, msg, strlen(msg));

                write(file_fd, buffer + 4, received_bytes - 4);

                uint16_t ack_opcode = htons(4);
                uint16_t ack_block = htons(current_block);
                memcpy(buffer, &ack_opcode, sizeof(uint16_t));
                memcpy(buffer + 2, &ack_block, sizeof(uint16_t));

                sent_bytes = sendto(sock, buffer, 4, 0, (struct sockaddr *)&sender_addr, sender_len);
                if (sent_bytes == -1) {
                    write_message(STDOUT_FILENO, "Error sending ACK\n");
                    break;
                }
            }

        } else if (opcode == 5) {  // ERROR
            write_message(STDOUT_FILENO, "Error received from server: ");
            write_message(STDOUT_FILENO, buffer + 4);
            write_message(STDOUT_FILENO, "\n");
            break;
        }

        if (received_bytes < BUFFER_SIZE) {
            write_message(STDOUT_FILENO, "File transfer complete\n");
            break;
        }
    }

    close(file_fd);
    close(sock);
    freeaddrinfo(res);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        write_message(STDOUT_FILENO, "Usage: gettftp <server> <file>\n");
        exit(EXIT_FAILURE);
    }

    const char *server = argv[1];
    const char *file = argv[2];
    send_rrq_and_receive(server, file);

    return 0;
}
