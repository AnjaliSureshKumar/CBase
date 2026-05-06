#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8888
#define MAX_CLIENTS 2
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    int id;
} client_t;

static void broadcast_message(int sender_fd, client_t *clients, int max_clients, const char *message, size_t len) {
    for (int i = 0; i < max_clients; i++) {
        int client_fd = clients[i].fd;
        if (client_fd != -1 && client_fd != sender_fd) {
            ssize_t sent = send(client_fd, message, len, 0);
            if (sent < 0) {
                perror("send");
            }
        }
    }
}

int main(void) {
    int listen_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    client_t clients[MAX_CLIENTS];
    fd_set readfds;
    int max_fd;
    char buffer[BUFFER_SIZE];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].id = 0;
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

    if (listen(listen_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    printf("Chat server listening on port %d\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(listen_fd, &readfds);
        max_fd = listen_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > max_fd) {
                    max_fd = clients[i].fd;
                }
            }
        }

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(listen_fd, &readfds)) {
            client_len = sizeof(client_addr);
            client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
            if (client_fd < 0) {
                perror("accept");
                continue;
            }

            int added = 0;
            int client_id = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == -1) {
                    clients[i].fd = client_fd;
                    clients[i].id = i + 1;
                    client_id = clients[i].id;
                    added = 1;
                    break;
                }
            }
            if (!added) {
                const char *msg = "Server full. Connection refused.\n";
                send(client_fd, msg, strlen(msg), 0);
                close(client_fd);
            } else {
                snprintf(buffer, sizeof(buffer), "WELCOME %d\n", client_id);
                send(client_fd, buffer, strlen(buffer), 0);
                snprintf(buffer, sizeof(buffer), "Client %d joined the chat.\n", client_id);
                printf("%s", buffer);
                broadcast_message(client_fd, clients, MAX_CLIENTS, buffer, strlen(buffer));
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = clients[i].fd;
            if (fd == -1) {
                continue;
            }

            if (FD_ISSET(fd, &readfds)) {
                ssize_t len = recv(fd, buffer, sizeof(buffer) - 1, 0);
                if (len <= 0) {
                    if (len < 0) perror("recv");
                    int disconnected_id = clients[i].id;
                    close(fd);
                    clients[i].fd = -1;
                    clients[i].id = 0;
                    snprintf(buffer, sizeof(buffer), "Client %d disconnected.\n", disconnected_id);
                    printf("%s", buffer);
                    broadcast_message(fd, clients, MAX_CLIENTS, buffer, strlen(buffer));
                } else {
                    buffer[len] = '\0';
                    char message[BUFFER_SIZE];
                    snprintf(message, sizeof(message), "Client %d: %s", clients[i].id, buffer);
                    broadcast_message(fd, clients, MAX_CLIENTS, message, strlen(message));
                }
            }
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) close(clients[i].fd);
    }
    close(listen_fd);
    return EXIT_SUCCESS;
}
