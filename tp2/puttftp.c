#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>

#define BUFFER_SIZE 516
#define DATA_SIZE 512
#define TFTP_OPCODE_SIZE 2
#define TFTP_NULL_TERMINATOR 1
// WRQ format: [2 bytes opcode][filename+'\0'][mode+'\0']

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

void build_wrq(char *buffer, const char *filename, const char *mode) {
    uint16_t opcode = htons(2); 
    size_t offset = 0;

    memcpy(buffer + offset, &opcode, TFTP_OPCODE_SIZE);
    offset += TFTP_OPCODE_SIZE;
    strcpy(buffer + offset, filename);
    offset += strlen(filename) + TFTP_NULL_TERMINATOR;
    strcpy(buffer + offset, mode);
    offset += strlen(mode) + TFTP_NULL_TERMINATOR;
    // WRQ packet built: opcode + filename+'\0' + mode+'\0'
}

void send_wrq_and_send_data(const char *server, const char *filename) {
    struct addrinfo *res;
    char buffer[BUFFER_SIZE];
    ssize_t sent_bytes, received_bytes;
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

    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1) {
        write_message(STDOUT_FILENO, "Error opening local file\n");
        close(sock);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    build_wrq(buffer, filename, "octet");

    size_t wrq_length = TFTP_OPCODE_SIZE 
                        + (strlen(filename) + TFTP_NULL_TERMINATOR) 
                        + (strlen("octet") + TFTP_NULL_TERMINATOR);

    sent_bytes = sendto(sock, buffer, wrq_length, 0, res->ai_addr, res->ai_addrlen);
    if (sent_bytes == -1) {
        write_message(STDOUT_FILENO, "Error sending WRQ\n");
        close(file_fd);
        close(sock);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    write_message(STDOUT_FILENO, "WRQ sent to server\n");

    struct sockaddr_in server_addr;
    socklen_t server_len = sizeof(server_addr);

    received_bytes = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &server_len);
    if (received_bytes == -1) {
        write_message(STDOUT_FILENO, "Error receiving ACK for WRQ\n");
        close(file_fd);
        close(sock);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    uint16_t opcode, block_num;
    memcpy(&opcode, buffer, sizeof(uint16_t));
    opcode = ntohs(opcode);

    if (opcode == 5) {
        write_message(STDOUT_FILENO, "Error received from server: ");
        write_message(STDOUT_FILENO, buffer + 4);
        write_message(STDOUT_FILENO, "\n");
        close(file_fd);
        close(sock);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    if (opcode != 4) {
        write_message(STDOUT_FILENO, "Expected ACK, got something else\n");
        close(file_fd);
        close(sock);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    memcpy(&block_num, buffer + 2, sizeof(uint16_t));
    block_num = ntohs(block_num);

    if (block_num != 0) {
        write_message(STDOUT_FILENO, "Expected ACK block 0\n");
        close(file_fd);
        close(sock);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    write_message(STDOUT_FILENO, "ACK for WRQ received, starting data transfer\n");

    char file_data_buffer[DATA_SIZE];
    ssize_t read_bytes;
    current_block = 1;

    while ((read_bytes = read(file_fd, file_data_buffer, DATA_SIZE)) > 0) {
        uint16_t data_opcode = htons(3);
        uint16_t net_block = htons(current_block);

        memcpy(buffer, &data_opcode, sizeof(uint16_t));
        memcpy(buffer + 2, &net_block, sizeof(uint16_t));
        memcpy(buffer + 4, file_data_buffer, (size_t)read_bytes);

        sent_bytes = sendto(sock, buffer, (size_t)read_bytes + 4, 0, (struct sockaddr *)&server_addr, server_len);
        if (sent_bytes == -1) {
            write_message(STDOUT_FILENO, "Error sending DATA packet\n");
            break;
        }

        received_bytes = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &server_len);
        if (received_bytes == -1) {
            write_message(STDOUT_FILENO, "Error receiving ACK\n");
            break;
        }

        memcpy(&opcode, buffer, sizeof(uint16_t));
        opcode = ntohs(opcode);

        if (opcode == 5) {
            write_message(STDOUT_FILENO, "Error received from server: ");
            write_message(STDOUT_FILENO, buffer + 4);
            write_message(STDOUT_FILENO, "\n");
            break;
        }

        if (opcode != 4) {
            write_message(STDOUT_FILENO, "Expected ACK, got something else\n");
            break;
        }

        memcpy(&block_num, buffer + 2, sizeof(uint16_t));
        block_num = ntohs(block_num);

        if (block_num != current_block) {
            write_message(STDOUT_FILENO, "Received ACK for unexpected block\n");
            break;
        }

        current_block++;

        // If last block < DATA_SIZE, end of file
        if (read_bytes < DATA_SIZE) {
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
        write_message(STDERR_FILENO, "Usage: puttftp <server> <file>\n");
        exit(EXIT_FAILURE);
    }

    const char *server = argv[1];
    const char *file = argv[2];
    send_wrq_and_send_data(server, file);

    return 0;
}