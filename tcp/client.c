#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8888
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char input[BUFFER_SIZE];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IP address
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("invalid address or address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server at %s:%d\n", SERVER_IP, PORT);
    printf("Type messages to send (type 'quit' to exit):\n");

    while (1) {
        // Get input from user
        printf("\nYou: ");
        fgets(input, BUFFER_SIZE, stdin);

        // Check for quit command
        if (strncmp(input, "quit", 4) == 0) {
            printf("Disconnecting...\n");
            break;
        }

        // Send message to server
        if (send(sock, input, strlen(input), 0) < 0) {
            perror("send failed");
            break;
        }

        // Receive response from server
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_read = recv(sock, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_read < 0) {
            perror("recv failed");
            break;
        }

        if (bytes_read == 0) {
            printf("Server closed connection\n");
            break;
        }

        printf("Server: %s", buffer);
    }

    close(sock);
    return 0;
}
