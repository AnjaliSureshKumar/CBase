#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#define PORT 7779
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void broadcast_message(int sender_fd, int *clients, int max_clients, const char *message, size_t len) {
    for (int i = 0; i < max_clients; i++) {
        int fd = clients[i];
        if (fd != -1 && fd != sender_fd) {
            ssize_t sent = send(fd, message, len, 0);
            if (sent < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
                perror("send");
            }
        }
    }
}

int main(void) {
    int listen_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int clients[MAX_CLIENTS];
    fd_set readfds;
    int max_fd;
    char buffer[BUFFER_SIZE];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = -1;
    }

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    if (listen(listen_fd, 10) < 0) {
        perror("listen");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    if (set_nonblocking(listen_fd) < 0) {
        perror("set_nonblocking");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    printf("Chat server listening on port %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(listen_fd, &readfds);
        max_fd = listen_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != -1) {
                FD_SET(clients[i], &readfds);
                if (clients[i] > max_fd) {
                    max_fd = clients[i];
                }
            }
        }

        int ready = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (ready < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(listen_fd, &readfds)) {
            client_len = sizeof(client_addr);
            client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
            if (client_fd < 0) {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    perror("accept");
                }
            } else {
                if (set_nonblocking(client_fd) < 0) {
                    perror("set_nonblocking");
                    close(client_fd);
                } else {
                    int added = 0;
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (clients[i] == -1) {
                            clients[i] = client_fd;
                            added = 1;
                            break;
                        }
                    }
                    if (!added) {
                        const char *msg = "Server is full. Try again later.\n";
                        send(client_fd, msg, strlen(msg), 0);
                        close(client_fd);
                    } else {
                        snprintf(buffer, sizeof(buffer), "New user joined: %s:%d\n",
                                 inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                        printf("%s", buffer);
                        broadcast_message(client_fd, clients, MAX_CLIENTS, buffer, strlen(buffer));
                    }
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = clients[i];
            if (fd == -1) continue;

            if (FD_ISSET(fd, &readfds)) {
                ssize_t bytes_read = recv(fd, buffer, sizeof(buffer) - 1, 0);
                if (bytes_read <= 0) {
                    if (bytes_read < 0) perror("recv");
                    close(fd);
                    clients[i] = -1;
                    snprintf(buffer, sizeof(buffer), "A user disconnected (fd %d)\n", fd);
                    printf("%s", buffer);
                    broadcast_message(fd, clients, MAX_CLIENTS, buffer, strlen(buffer));
                } else {
                    buffer[bytes_read] = '\0';
                    char message[BUFFER_SIZE + 64];
                    snprintf(message, sizeof(message), "User %d: %s", fd, buffer);
                    broadcast_message(fd, clients, MAX_CLIENTS, message, strlen(message));
                }
            }
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1) close(clients[i]);
    }
    close(listen_fd);
    return EXIT_SUCCESS;
}
