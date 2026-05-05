#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 9999
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t server_addr_len;
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    int max_fd;

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IP address
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("invalid address or address not supported");
        exit(EXIT_FAILURE);
    }

    server_addr_len = sizeof(server_addr);

    printf("UDP Client ready to communicate with server at %s:%d\n", SERVER_IP, PORT);
    printf("Type messages to send (type 'quit' to exit):\n\n");

    // Main client loop with select for multiplexing
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);              // Monitor UDP socket for incoming messages
        FD_SET(STDIN_FILENO, &readfds);        // Monitor stdin for user input

        max_fd = sockfd;

        printf("You: ");
        fflush(stdout);

        // Wait for activity on any of the file descriptors
        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select failed");
            exit(EXIT_FAILURE);
        }

        // Check for incoming UDP message from server
        if (FD_ISSET(sockfd, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            ssize_t bytes_read = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                                        (struct sockaddr *)&server_addr, &server_addr_len);

            if (bytes_read < 0) {
                perror("recvfrom failed");
            } else {
                printf("\nServer: %s", buffer);
            }
        }

        // Check for user input from stdin
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
                // Check for quit command
                if (strncmp(buffer, "quit", 4) == 0) {
                    printf("Disconnecting...\n");
                    break;
                }

                // Send message to server
                if (sendto(sockfd, buffer, strlen(buffer), 0,
                          (struct sockaddr *)&server_addr, server_addr_len) < 0) {
                    perror("sendto failed");
                }
            }
        }
    }

    close(sockfd);
    return 0;
}
