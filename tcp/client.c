#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8888
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

int main(void) {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    char input[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket creation failed"); exit(EXIT_FAILURE); }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) { perror("invalid address or address not supported"); exit(EXIT_FAILURE); }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { perror("connection failed"); exit(EXIT_FAILURE); }

    printf("Connected to server at %s:%d\nType messages to send (type 'quit' to exit):\n", SERVER_IP, PORT);

    while (1) {
        printf("\nYou: ");
        if (!fgets(input, BUFFER_SIZE, stdin)) break;
        if (strncmp(input, "quit", 4) == 0) { printf("Disconnecting...\n"); break; }
        if (send(sock, input, strlen(input), 0) < 0) { perror("send failed"); break; }

        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_read = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read < 0) { perror("recv failed"); break; }
        if (bytes_read == 0) { printf("Server closed connection\n"); break; }

        printf("Server: %s", buffer);
    }

    close(sock);
    return 0;
}
