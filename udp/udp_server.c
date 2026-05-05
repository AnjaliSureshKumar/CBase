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

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    int max_fd;

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind socket to port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("UDP Server listening on port %d...\n", PORT);
    printf("Waiting for client messages...\n");
    printf("(Type messages to send to last connected client, or 'quit' to exit)\n\n");

    client_addr_len = sizeof(client_addr);
    int has_client = 0;

    // Main server loop with select for multiplexing
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);              // Monitor UDP socket for incoming messages
        FD_SET(STDIN_FILENO, &readfds);        // Monitor stdin for user input

        max_fd = sockfd;

        // Wait for activity on any of the file descriptors
        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select failed");
            exit(EXIT_FAILURE);
        }

        // Check for incoming UDP message
        if (FD_ISSET(sockfd, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            ssize_t bytes_read = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                                        (struct sockaddr *)&client_addr, &client_addr_len);

            if (bytes_read < 0) {
                perror("recvfrom failed");
            } else {
                printf("\n[From %s:%d]: %s",
                       inet_ntoa(client_addr.sin_addr),
                       ntohs(client_addr.sin_port), buffer);
                has_client = 1;
                printf("You: ");
                fflush(stdout);
            }
        }

        // Check for user input from stdin
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
                // Check for quit command
                if (strncmp(buffer, "quit", 4) == 0) {
                    printf("Shutting down UDP server...\n");
                    break;
                }

                if (has_client) {
                    if (sendto(sockfd, buffer, strlen(buffer), 0,
                              (struct sockaddr *)&client_addr, client_addr_len) < 0) {
                        perror("sendto failed");
                    } else {
                        printf("Message sent to %s:%d\n",
                               inet_ntoa(client_addr.sin_addr),
                               ntohs(client_addr.sin_port));
                    }
                } else {
                    printf("No client has sent a message yet. Message not sent.\n");
                }
                printf("You: ");
                fflush(stdout);
            }
        }
    }

    close(sockfd);
    return 0;
}
