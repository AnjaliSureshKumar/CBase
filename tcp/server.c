#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8888
#define BUFFER_SIZE 1024
#define BACKLOG 5

int main(void) {
    int server_fd, client_fd = -1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    int max_fd;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket creation failed"); exit(EXIT_FAILURE); }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { perror("setsockopt failed"); exit(EXIT_FAILURE); }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) { perror("bind failed"); exit(EXIT_FAILURE); }
    if (listen(server_fd, BACKLOG) < 0) { perror("listen failed"); exit(EXIT_FAILURE); }

    printf("Server listening on port %d...\nWaiting for client connection...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        max_fd = server_fd;
        if (client_fd != -1) { FD_SET(client_fd, &readfds); if (client_fd > max_fd) max_fd = client_fd; }

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) { perror("select failed"); exit(EXIT_FAILURE); }

        if (FD_ISSET(server_fd, &readfds)) {
            if (client_fd != -1) close(client_fd);
            client_addr_len = sizeof(client_addr);
            client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client_fd < 0) perror("accept failed");
            else { printf("Client connected from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port)); fflush(stdout); }
        }

        if (client_fd != -1 && FD_ISSET(client_fd, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            ssize_t bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_read <= 0) {
                if (bytes_read < 0) perror("recv failed");
                printf("Client disconnected\n");
                close(client_fd);
                client_fd = -1;
            } else {
                printf("\nClient: %sYou: ", buffer);
                fflush(stdout);
            }
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) continue;
            if (strncmp(buffer, "quit", 4) == 0) { printf("Shutting down server...\n"); if (client_fd != -1) close(client_fd); break; }
            if (client_fd != -1) {
                if (send(client_fd, buffer, strlen(buffer), 0) < 0) { perror("send failed"); close(client_fd); client_fd = -1; }
            } else {
                printf("No client connected. Message not sent.\nWaiting for client connection...\n");
            }
        }
    }

    if (client_fd != -1) close(client_fd);
    close(server_fd);
    return 0;
}
